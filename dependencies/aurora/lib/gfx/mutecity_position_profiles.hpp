#pragma once
#include <cstdint>
namespace aurora::gfx::mutecity_position_profiles {
struct Profile { uint64_t shader; unsigned stride, fraction, indexWidth, arrayStride; bool floats; };
inline constexpr Profile profiles[] = {
{246249707ull,3u,9u,1u,6u,false},
{268593618ull,3u,9u,1u,6u,false},
{1258351285ull,6u,0u,2u,12u,true},
{1368612553ull,3u,10u,1u,6u,false},
{1511415406ull,3u,10u,1u,6u,false},
{1976919269ull,6u,0u,2u,12u,true},
{2157421414ull,3u,9u,1u,6u,false},
{2157786923ull,2u,10u,1u,6u,false},
{2357105898ull,4u,7u,1u,6u,false},
{2550567221ull,6u,0u,2u,12u,true},
{2600238719ull,4u,7u,1u,6u,false},
{2721962110ull,2u,10u,1u,6u,false},
{2840107346ull,4u,7u,1u,6u,false},
{2918264976ull,6u,0u,2u,12u,true},
{2954660129ull,6u,0u,2u,12u,true},
{3216646290ull,2u,7u,1u,6u,false},
{3458556844ull,4u,0u,2u,12u,true},
{3584684015ull,3u,9u,1u,6u,false},
{3779601551ull,3u,10u,1u,6u,false},
{3983723562ull,3u,10u,1u,6u,false},
{4091811771ull,4u,7u,1u,6u,false},
{4254603670ull,3u,9u,1u,6u,false},
{4273269501ull,6u,0u,2u,12u,true},
};
inline const Profile* find(uint64_t shader) { for(const auto& p:profiles) if(p.shader==shader) return &p; return nullptr; }
}
