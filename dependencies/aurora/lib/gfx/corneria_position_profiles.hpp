#pragma once
#include <cstdint>
namespace aurora::gfx::corneria_position_profiles {
struct Profile {uint64_t shader; unsigned stride, fraction;};
inline constexpr Profile profiles[] = {
{1719209447ull,8u,6u},
{1059453435ull,10u,6u},
{3786357780ull,4u,6u},
{1853503396ull,6u,6u},
{648529937ull,8u,6u},
{1065832977ull,6u,6u},
};
inline const Profile* find(uint64_t shader) {for(const auto& p:profiles)if(p.shader==shader)return &p;return nullptr;}
}
