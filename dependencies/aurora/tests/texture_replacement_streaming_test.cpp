#include "gfx/dds_io.hpp"
#include "gfx/png_io.hpp"
#include "gfx/texture_replacement.hpp"

#include <aurora/texture.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace aurora::gfx::texture_replacement {
namespace {
using namespace std::chrono_literals;

constexpr const char* VirtualPathA = "tex1_4x4_0000000000000001_6.dds";
constexpr const char* VirtualPathB = "tex1_4x4_0000000000000002_6.dds";
constexpr const char* VirtualPngPath = "tex1_4x4_0000000000000001_6.png";

struct BlockingSource {
  std::mutex mutex;
  std::condition_variable cv;
  bool entered = false;
  bool release = false;
  std::atomic<uint32_t> reads = 0;

  static bool read(void* userData, const char*, std::vector<uint8_t>&) {
    auto& source = *static_cast<BlockingSource*>(userData);
    ++source.reads;
    std::unique_lock lock{source.mutex};
    source.entered = true;
    source.cv.notify_all();
    source.cv.wait(lock, [&] { return source.release; });
    return false;
  }
};

struct RecordingSource {
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::string> paths;

  static bool read(void* userData, const char* path, std::vector<uint8_t>&) {
    auto& source = *static_cast<RecordingSource*>(userData);
    {
      std::lock_guard lock{source.mutex};
      source.paths.emplace_back(path);
    }
    source.cv.notify_all();
    return false;
  }
};

class ReplacementStreamingTest : public ::testing::Test {
protected:
  void SetUp() override {
    shutdown();
    testing::set_worker_count(1);
  }

  void TearDown() override { shutdown(); }
};

TEST_F(ReplacementStreamingTest, UnregisterWaitsForInFlightVirtualRead) {
  BlockingSource source;
  const auto registration =
      texture::register_virtual_replacement(VirtualPathA, {.read = &BlockingSource::read, .userData = &source});
  ASSERT_NE(registration.id, 0u);
  {
    std::unique_lock lock{source.mutex};
    ASSERT_TRUE(source.cv.wait_for(lock, 2s, [&] { return source.entered; }));
  }

  auto unregister = std::async(std::launch::async, [&] { texture::unregister_replacement(registration); });
  EXPECT_EQ(unregister.wait_for(50ms), std::future_status::timeout);
  {
    std::lock_guard lock{source.mutex};
    source.release = true;
  }
  source.cv.notify_all();

  EXPECT_EQ(unregister.wait_for(2s), std::future_status::ready);
  EXPECT_EQ(source.reads.load(), 1u);
}

TEST_F(ReplacementStreamingTest, CancelledQueuedVirtualJobNeverReads) {
  RecordingSource source;
  testing::set_workers_paused(true);
  const auto registration =
      texture::register_virtual_replacement(VirtualPathA, {.read = &RecordingSource::read, .userData = &source});
  ASSERT_NE(registration.id, 0u);

  texture::unregister_replacement(registration);
  testing::set_workers_paused(false);
  shutdown();

  EXPECT_TRUE(source.paths.empty());
}

TEST_F(ReplacementStreamingTest, HighPriorityFullLoadRunsBeforeThumbnailQueue) {
  RecordingSource source;
  testing::set_workers_paused(true);
  const auto low =
      texture::register_virtual_replacement(VirtualPathA, {.read = &RecordingSource::read, .userData = &source});
  const auto high =
      texture::register_virtual_replacement(VirtualPathB, {.read = &RecordingSource::read, .userData = &source});
  ASSERT_NE(low.id, 0u);
  ASSERT_NE(high.id, 0u);

  GXTexObj_ obj{};
  const auto key = std::get<texture::TextureSourceKey>(high.key);
  ASSERT_TRUE(find_source_replacement(obj, key).has_value());
  testing::set_workers_paused(false);
  {
    std::unique_lock lock{source.mutex};
    ASSERT_TRUE(source.cv.wait_for(lock, 2s, [&] { return !source.paths.empty(); }));
    EXPECT_EQ(source.paths.front(), VirtualPathB);
  }
  const std::array registrations{low, high};
  texture::unregister_replacements(registrations);
}

TEST_F(ReplacementStreamingTest, FullLoadAdmissionDefersAndRetriesWithoutBlockingOwner) {
  RecordingSource source;
  testing::set_workers_paused(true);
  std::vector<texture::ReplacementRegistration> registrations;
  for (uint32_t i = 1; i <= 5; ++i) {
    const auto path = "tex1_4x4_000000000000000" + std::to_string(i) + "_6.png";
    registrations.push_back(texture::register_virtual_replacement(
        path, {.read = &RecordingSource::read, .userData = &source}));
    ASSERT_NE(registrations.back().id, 0u);
    GXTexObj_ obj{};
    EXPECT_TRUE(find_source_replacement(obj, std::get<texture::TextureSourceKey>(registrations.back().key)).has_value());
  }
  EXPECT_EQ(testing::pending_full_loads(), 4u);
  testing::set_workers_paused(false);
  for (size_t i = 0; i < 4; ++i) {
    ASSERT_TRUE(testing::wait_for_completions(registrations[i].id, 1, 2000));
  }
  process_streaming(); // Failed fixture reads release all admission slots, without a GPU.
  EXPECT_EQ(testing::pending_full_loads(), 0u);
  GXTexObj_ obj{};
  EXPECT_TRUE(find_source_replacement(obj, std::get<texture::TextureSourceKey>(registrations.back().key)).has_value());
  EXPECT_TRUE(testing::wait_for_completions(registrations.back().id, 1, 2000));
  texture::unregister_replacements(registrations);
}

TEST(PngDecodedBudgetTest, ExactLimitAcceptedAndSmallerLimitRejected) {
  const std::array<uint8_t, 70> redPixel = {
      137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,
      0,0,0,13,73,68,65,84,120,156,99,248,207,192,240,31,0,5,0,1,255,137,153,61,29,
      0,0,0,0,73,69,78,68,174,66,96,130};
  EXPECT_FALSE(png::parse_png_bytes({redPixel.data(), redPixel.size()}, 3).has_value());
  const auto accepted = png::parse_png_bytes({redPixel.data(), redPixel.size()}, 4);
  ASSERT_TRUE(accepted.has_value());
  ASSERT_EQ(accepted->data.size(), 4u);
  EXPECT_EQ(accepted->data.data()[0], 255u);
  EXPECT_EQ(accepted->data.data()[1], 0u);
  EXPECT_EQ(accepted->data.data()[2], 0u);
  EXPECT_EQ(accepted->data.data()[3], 255u);
}

TEST_F(ReplacementStreamingTest, StaleFailureDoesNotPoisonNewRegistration) {
  RecordingSource source;
  testing::set_workers_paused(true);
  const auto oldRegistration =
      texture::register_virtual_replacement(VirtualPngPath, {.read = &RecordingSource::read, .userData = &source});
  ASSERT_NE(oldRegistration.id, 0u);
  GXTexObj_ obj{};
  const auto key = std::get<texture::TextureSourceKey>(oldRegistration.key);
  ASSERT_TRUE(find_source_replacement(obj, key).has_value());
  testing::set_workers_paused(false);
  ASSERT_TRUE(testing::wait_for_completions(oldRegistration.id, 1, 2000));
  texture::unregister_replacement(oldRegistration);

  testing::set_workers_paused(true);
  const auto newRegistration =
      texture::register_virtual_replacement(VirtualPngPath, {.read = &RecordingSource::read, .userData = &source});
  process_streaming();

  ASSERT_TRUE(find_source_replacement(obj, key).has_value());
  testing::set_workers_paused(false);
  EXPECT_TRUE(testing::wait_for_completions(newRegistration.id, 1, 2000));
  texture::unregister_replacement(newRegistration);
}

template <typename T>
void write_u32(std::vector<uint8_t>& bytes, size_t offset, T value) {
  const uint32_t word = static_cast<uint32_t>(value);
  std::memcpy(bytes.data() + offset, &word, sizeof(word));
}

TEST(ImportedPngMipTest, ValidatedAuthoredChainsDecodeWithNativeFileLoader) {
  const char* directory = std::getenv("AURORA_TEST_HD_MIP_ROOT");
  if (!directory || !*directory) GTEST_SKIP() << "Local authored HD assets not installed";
  struct Case { const char* file; uint32_t width, height, mips; };
  const std::array<Case, 8> cases{{
      {"tex1_32x128_ae8a1ac86ffa7d6e_0.png", 128, 512, 10},
      {"tex1_128x128_m_57e21bef87cb26ca_14.png", 512, 512, 10},
      {"tex1_128x128_m_b55385186fe908c8_14.png", 512, 512, 10},
      {"tex1_256x256_m_5d92246bb24244b0_14.png", 1024, 1024, 11},
      {"tex1_256x256_m_ea3215945fad62b4_14.png", 256, 256, 9},
      {"tex1_32x32_m_02d36070f2bb6e82_14.png", 640, 640, 1},
      {"tex1_32x32_m_59ab8f574e895183_14.png", 640, 640, 1},
      {"tex1_32x32_m_fc4996a1aced072a_14.png", 640, 640, 1},
  }};
  for (const auto& item : cases) {
    SCOPED_TRACE(item.file);
    testing::DecodedReplacementInfo info;
    ASSERT_TRUE(testing::inspect_file_replacement(std::filesystem::path(directory) / item.file, info));
    EXPECT_EQ(info.width, item.width);
    EXPECT_EQ(info.height, item.height);
    EXPECT_EQ(info.mips, item.mips);
    uint64_t bytes = 0;
    for (uint32_t level = 0; level < item.mips; ++level)
      bytes += uint64_t(std::max(1u, item.width >> level)) * std::max(1u, item.height >> level) * 4;
    EXPECT_EQ(info.bytes, bytes);
  }
}

TEST(DdsMipTailTest, SlicesTheSameTailAsFullDecode) {
  std::vector<uint8_t> base(8 * 8 * 4);
  auto encoded = dds::encode_rgba8_dds(8, 8, base);
  std::vector<uint8_t> bytes(encoded.data(), encoded.data() + encoded.size());
  bytes.resize(128 + (8 * 8 + 4 * 4 + 2 * 2 + 1) * 4);
  for (size_t i = 128; i < bytes.size(); ++i) {
    bytes[i] = static_cast<uint8_t>(i);
  }
  write_u32(bytes, 8, 0x1u | 0x2u | 0x4u | 0x8u | 0x1000u | 0x20000u);
  write_u32(bytes, 28, 4u);
  write_u32(bytes, 108, 0x1000u | 0x8u | 0x400000u);

  const auto full = dds::parse_dds_bytes({bytes.data(), bytes.size()});
  const auto tail = dds::parse_dds_mip_tail({bytes.data(), bytes.size()}, 2);
  const auto path =
      std::filesystem::temp_directory_path() /
      ("aurora-dds-tail-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dds");
  {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  const auto fileTail = dds::load_dds_mip_tail(path, 2);
  std::error_code removeError;
  std::filesystem::remove(path, removeError);
  ASSERT_TRUE(full.has_value());
  ASSERT_TRUE(tail.has_value());
  ASSERT_TRUE(fileTail.has_value());
  EXPECT_FALSE(tail->includesBase);
  EXPECT_EQ(tail->texture.width, 2u);
  EXPECT_EQ(tail->texture.height, 2u);
  EXPECT_EQ(tail->texture.mips, 2u);
  ASSERT_EQ(tail->texture.data.size(), 20u);
  EXPECT_TRUE(std::equal(tail->texture.data.data(), tail->texture.data.data() + tail->texture.data.size(),
                         full->data.data() + full->data.size() - 20));
  ASSERT_EQ(fileTail->texture.data.size(), tail->texture.data.size());
  EXPECT_TRUE(std::equal(fileTail->texture.data.data(), fileTail->texture.data.data() + fileTail->texture.data.size(),
                         tail->texture.data.data()));
}

TEST(DdsMipTailTest, RejectsMiplessLargeTextureButKeepsSmallWholeTexture) {
  std::vector<uint8_t> largePixels(128 * 128 * 4);
  const auto large = dds::encode_rgba8_dds(128, 128, largePixels);
  EXPECT_FALSE(dds::parse_dds_mip_tail(large, 64).has_value());

  std::vector<uint8_t> smallPixels(32 * 16 * 4);
  const auto small = dds::encode_rgba8_dds(32, 16, smallPixels);
  const auto tail = dds::parse_dds_mip_tail(small, 64);
  ASSERT_TRUE(tail.has_value());
  EXPECT_TRUE(tail->includesBase);
  ASSERT_EQ(tail->texture.data.size(), smallPixels.size());
  EXPECT_TRUE(
      std::equal(tail->texture.data.data(), tail->texture.data.data() + tail->texture.data.size(), smallPixels.data()));
}
} // namespace
} // namespace aurora::gfx::texture_replacement
