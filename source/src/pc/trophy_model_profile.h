#ifndef MELEE_PC_TROPHY_MODEL_PROFILE_H
#define MELEE_PC_TROPHY_MODEL_PROFILE_H
/* gm_1601 campaign ID tables -> ToyModelFile/US overrides -> gm_19EF Joint.
 * Exact source-audited envelopes for 26 standard forms across three modes.
 * Boss/wireframe/standalone Popo entries are excluded. Each export retains
 * one null-animation model descriptor and its terminated model list;
 * StaticModelDesc/DynamicModelDesc agree in this null-animation layout.
 * Every admitted file still requires complete typed graph ownership. */
static int MeleePc_IsStandardTrophyModelProfile(
    unsigned char* bytes, uint32_t body, uint32_t relocs, uint32_t publics,
    size_t roots, size_t strings)
{
    static const struct { uint32_t length, body, relocs, root; const char* symbol; } profiles[] = {
        /* Classic CKind_Captain */
        {722699,718024,1152,348904,"ToyCaptainModel_TopN_joint"},
        /* Classic CKind_Donkey */
        {625062,624056,235,411272,"ToyDonkeyModel_TopN_joint"},
        /* Classic CKind_Fox */
        {125747,124600,271,41768,"ToyFoxModel_TopN_joint"},
        /* Classic CKind_GameWatch */
        {51813,51456,72,50856,"ToyGamewatchModel_TopN_joint"},
        /* Classic CKind_Kirby */
        {174805,174552,47,45032,"ToyKirbyModel_TopN_joint"},
        /* Classic CKind_Koopa */
        {376893,372984,961,356488,"ToyKoopaModel_TopN_joint"},
        /* Classic CKind_Link */
        {766740,764088,647,113160,"ToyLinkModel_TopN_joint"},
        /* Classic CKind_Luigi */
        {1423685,1421944,419,92424,"ToyLuigiModel_TopN_joint"},
        /* Classic CKind_Mario */
        {313593,312792,184,66536,"ToyMarioModel_TopN_joint"},
        /* Classic CKind_Mars */
        {446208,444520,406,146568,"ToyMarsModel_TopN_joint"},
        /* Classic CKind_Mewtwo */
        {555962,555064,208,117320,"ToyMewtwoModel_TopN_joint"},
        /* Classic CKind_Ness */
        {256940,255624,313,81800,"ToyNessModel_TopN_joint"},
        /* Classic CKind_Peach */
        {990213,988472,419,185096,"ToyPeachModel_TopN_joint"},
        /* Classic CKind_Pikachu */
        {115855,115384,101,57288,"ToyPikachuModel_TopN_joint"},
        /* Classic CKind_PopoNana */
        {10170,9912,47,1224,"ToyIceclimberModel_TopN_joint"},
        /* Classic CKind_Purin */
        {90001,89560,94,60840,"ToyPurinModel_TopN_joint"},
        /* Classic CKind_Samus */
        {498053,495928,515,103112,"ToySamusModel_TopN_joint"},
        /* Classic CKind_Yoshi */
        {398857,397720,268,89672,"ToyYoshiModel_TopN_joint"},
        /* Classic CKind_Zelda */
        {397085,394728,573,99688,"ToyZeldaModel_TopN_joint"},
        /* Classic CKind_Seak */
        {1337016,1335064,472,98088,"ToySeakModel_TopN_joint"},
        /* Classic CKind_Falco */
        {53665,52568,258,26568,"ToyFalcoModel_TopN_joint"},
        /* Classic CKind_CLink */
        {744701,742712,481,106056,"ToyClinkModel_TopN_joint"},
        /* Classic CKind_DrMario */
        {451059,449976,254,84936,"ToyDrmarioModel_TopN_joint"},
        /* Classic CKind_Emblem */
        {409838,407000,693,141480,"ToyEmblemModel_TopN_joint"},
        /* Classic CKind_Pichu */
        {114509,113752,173,42920,"ToyPichuModel_TopN_joint"},
        /* Classic CKind_Ganon */
        {831941,827944,982,174984,"ToyGanondorfModel_TopN_joint"},
        /* Adventure CKind_Captain */
        {538416,536440,477,151944,"ToyCaptainRModel_TopN_joint"},
        /* Adventure CKind_Donkey */
        {468839,467800,243,94504,"ToyDonkeyRModel_TopN_joint"},
        /* Adventure CKind_Fox */
        {469984,467672,562,104584,"ToyFoxRModel_TopN_joint"},
        /* Adventure CKind_GameWatch */
        {38742,38560,28,38088,"ToyGamewatchRModel_TopN_joint"},
        /* Adventure CKind_Kirby */
        {81506,81144,74,40584,"ToyKirbyRModel_TopN_joint"},
        /* Adventure CKind_Koopa */
        {391498,389688,436,105832,"ToyKoopaRModel_TopN_joint"},
        /* Adventure CKind_Link */
        {360401,357720,654,154536,"ToyLinkRModel_TopN_joint"},
        /* Adventure CKind_Luigi */
        {516242,514488,422,93608,"ToyLuigiRModel_TopN_joint"},
        /* Adventure CKind_Mario */
        {274702,272728,477,98856,"ToyMarioRModel_TopN_joint"},
        /* Adventure CKind_Mars */
        {624801,622360,594,114792,"ToyMarsRModel_TopN_joint"},
        /* Adventure CKind_Mewtwo */
        {303107,302200,210,117896,"ToyMewtwoRModel_TopN_joint"},
        /* Adventure CKind_Ness */
        {306509,304632,453,100360,"ToyNessRModel_TopN_joint"},
        /* Adventure CKind_Peach */
        {673798,671816,479,143976,"ToyPeachRModel_TopN_joint"},
        /* Adventure CKind_Pikachu */
        {78204,77688,112,63816,"ToyPikachuRModel_TopN_joint"},
        /* Adventure CKind_PopoNana */
        {482323,479544,677,101192,"ToyIceclimberRModel_TopN_joint"},
        /* Adventure CKind_Purin */
        {89970,89528,94,60808,"ToyPurinRModel_TopN_joint"},
        /* Adventure CKind_Samus */
        {282090,279672,588,113160,"ToySamusRModel_TopN_joint"},
        /* Adventure CKind_Yoshi */
        {281758,280728,241,91272,"ToyYoshiRModel_TopN_joint"},
        /* Adventure CKind_Zelda */
        {437974,435992,479,102760,"ToyZeldaRModel_TopN_joint"},
        /* Adventure CKind_Seak */
        {556157,554200,473,94344,"ToySeakRModel_TopN_joint"},
        /* Adventure CKind_Falco */
        {239770,238680,256,74024,"ToyFalcoRModel_TopN_joint"},
        /* Adventure CKind_CLink */
        {297022,294712,561,106888,"ToyClinkRModel_TopN_joint"},
        /* Adventure CKind_DrMario */
        {429512,427224,555,104680,"ToyDrmarioRModel_TopN_joint"},
        /* Adventure CKind_Emblem */
        {480599,477400,783,116200,"ToyEmblemRModel_TopN_joint"},
        /* Adventure CKind_Pichu */
        {83934,83320,137,50024,"ToyPichuRModel_TopN_joint"},
        /* Adventure CKind_Ganon */
        {316198,313400,682,105128,"ToyGanondorfRModel_TopN_joint"},
        /* AllStar CKind_Captain */
        {540147,538200,469,153704,"ToyCaptainR2UsModel_TopN_joint"},
        /* AllStar CKind_Donkey */
        {465108,464024,254,90728,"ToyDonkeyR2Model_TopN_joint"},
        /* AllStar CKind_Fox */
        {312585,310648,468,97416,"ToyFoxR2Model_TopN_joint"},
        /* AllStar CKind_GameWatch */
        {164343,163768,126,163240,"ToyGamewatchR2Model_TopN_joint"},
        /* AllStar CKind_Kirby */
        {116047,115656,81,94120,"ToyKirbyR2Model_TopN_joint"},
        /* AllStar CKind_Koopa */
        {212379,211224,272,46888,"ToyKoopaR2Model_TopN_joint"},
        /* AllStar CKind_Link */
        {321430,318616,687,152936,"ToyLinkR2Model_TopN_joint"},
        /* AllStar CKind_Luigi */
        {240471,238872,383,89864,"ToyLuigiR2Model_TopN_joint"},
        /* AllStar CKind_Mario */
        {259651,257784,450,91848,"ToyMarioR2Model_TopN_joint"},
        /* AllStar CKind_Mars */
        {416366,414264,509,150824,"ToyMarsR2Model_TopN_joint"},
        /* AllStar CKind_Mewtwo */
        {156256,155256,233,84104,"ToyMewtwoR2Model_TopN_joint"},
        /* AllStar CKind_Ness */
        {317986,315960,490,102632,"ToyNessR2Model_TopN_joint"},
        /* AllStar CKind_Peach */
        {382119,379944,527,92456,"ToyPeachR2Model_TopN_joint"},
        /* AllStar CKind_Pikachu */
        {111293,110552,168,68296,"ToyPikachuR2Model_TopN_joint"},
        /* AllStar CKind_PopoNana */
        {428332,426360,475,101768,"ToyIceclimberR2Model_TopN_joint"},
        /* AllStar CKind_Purin */
        {100935,100344,131,63624,"ToyPurinR2Model_TopN_joint"},
        /* AllStar CKind_Samus */
        {47979,47512,100,29896,"ToySamusR2Model_TopN_joint"},
        /* AllStar CKind_Yoshi */
        {274997,273464,366,97000,"ToyYoshiR2Model_TopN_1_joint"},
        /* AllStar CKind_Zelda */
        {258023,255896,515,98920,"ToyZeldaR2Model_TopN_joint"},
        /* AllStar CKind_Seak */
        {250310,248248,499,94344,"ToySeakR2Model_TopN_joint"},
        /* AllStar CKind_Falco */
        {195551,194392,273,75624,"ToyFalcoR2Model_TopN_joint"},
        /* AllStar CKind_CLink */
        {262543,260312,541,104840,"ToyClinkR2Model_TopN_joint"},
        /* AllStar CKind_DrMario */
        {272537,270680,447,95432,"ToyDrmarioR2Model_TopN_joint"},
        /* AllStar CKind_Emblem */
        {475696,472568,765,113384,"ToyEmblemR2Model_TopN_joint"},
        /* AllStar CKind_Pichu */
        {158591,157144,345,74440,"ToyPichuR2Model_TopN_joint"},
        /* AllStar CKind_Ganon */
        {383979,380856,763,159688,"ToyGanondorfR2Model_TopN_joint"},
    };
    if (publics != 1 || MeleePc_DatBe32(bytes + 16) != 0) return 0;
    for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i)
        if (MeleePc_DatBe32(bytes) == profiles[i].length && body == profiles[i].body &&
            relocs == profiles[i].relocs && MeleePc_DatBe32(bytes + roots) == profiles[i].root &&
            !strcmp((char*) bytes + strings + MeleePc_DatBe32(bytes + roots + 4), profiles[i].symbol))
            return 1;
    return 0;
}
#endif
