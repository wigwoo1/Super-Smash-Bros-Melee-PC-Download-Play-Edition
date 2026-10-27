#ifndef MELEE_PC_ENDING_SCENE_PROFILE_H
#define MELEE_PC_ENDING_SCENE_PROFILE_H
/* Exact original GmRegEnd US/Japanese envelopes. gmregtyfall consumes the
 * first four SceneDesc roots; ftdemo consumes the remaining packed nested
 * motion archives. Descriptor and relocation ownership remain validated by
 * the existing title planner, with no generic body swapping. */
static int MeleePc_IsEndingSceneProfile(unsigned char* bytes,uint32_t body,
    uint32_t relocs,uint32_t publics,size_t roots,size_t strings)
{
    static const char* const names[31]={
        "cut1CanimScene",
        "cut2CanimScene",
        "cut3BgScene",
        "cut3CanimScene",
        "ftDemoEndingMotionFileCaptain",
        "ftDemoEndingMotionFileClink",
        "ftDemoEndingMotionFileDonkey",
        "ftDemoEndingMotionFileDrmario",
        "ftDemoEndingMotionFileEmblem",
        "ftDemoEndingMotionFileFalco",
        "ftDemoEndingMotionFileFox",
        "ftDemoEndingMotionFileGamewatch",
        "ftDemoEndingMotionFileGanon",
        "ftDemoEndingMotionFileKirby",
        "ftDemoEndingMotionFileKoopa",
        "ftDemoEndingMotionFileLink",
        "ftDemoEndingMotionFileLuigi",
        "ftDemoEndingMotionFileMario",
        "ftDemoEndingMotionFileMars",
        "ftDemoEndingMotionFileMewtwo",
        "ftDemoEndingMotionFileNana",
        "ftDemoEndingMotionFileNess",
        "ftDemoEndingMotionFilePeach",
        "ftDemoEndingMotionFilePichu",
        "ftDemoEndingMotionFilePikachu",
        "ftDemoEndingMotionFilePopo",
        "ftDemoEndingMotionFilePurin",
        "ftDemoEndingMotionFileSamus",
        "ftDemoEndingMotionFileSeak",
        "ftDemoEndingMotionFileYoshi",
        "ftDemoEndingMotionFileZelda",
    };
    static const uint32_t offsets[2][31]={
        {2596,5228,181508,8132,198048,230368,201984,194304,299840,211072,206784,280544,283232,214784,218016,224384,185792,181536,287296,292768,254848,236128,240224,251008,245920,258976,297152,262784,266944,276448,270112},
        {2596,5228,191316,8132,207872,240192,211808,204128,309664,220896,216608,290368,293056,224608,227840,234208,195616,191360,297120,302592,264672,245952,250048,260832,255744,268800,306976,272608,276768,286272,279936},
    };
    unsigned variant;
    if(publics!=31 || MeleePc_DatBe32(bytes+16)!=0)return 0;
    if(MeleePc_DatBe32(bytes)==312550 && body==304864 && relocs==1647)variant=0;
    else if(MeleePc_DatBe32(bytes)==322590 && body==314688 && relocs==1701)variant=1;
    else return 0;
    for(unsigned i=0;i<31;++i)
        if(MeleePc_DatBe32(bytes+roots+i*8)!=offsets[variant][i] ||
           strcmp((char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4),names[i]))return 0;
    return 1;
}
static int MeleePc_IsEndingDisplayProfile(unsigned char* bytes,uint32_t body,
    uint32_t relocs,uint32_t publics,size_t roots,size_t strings)
{
    static const char* const names[12]={
        "ToyDspMycharaCmA_Top_joint",
        "ToyDspMycharaCmA_Top_matanim_joint",
        "ToyDspMycharaCmB_Top_joint",
        "ToyDspMycharaCmB_Top_matanim_joint",
        "ToyDspMycharaCmC_Top_joint",
        "ToyDspMycharaCmC_Top_matanim_joint",
        "ToyDspMycharaCmD_Top_joint",
        "ToyDspMycharaCmD_Top_matanim_joint",
        "ToyDspMycharaCmE_Top_joint",
        "ToyDspMycharaCmE_Top_matanim_joint",
        "ToyDspStand_Top_joint",
        "ToyDspStand_Top_matanim_joint",
    };
    static const uint32_t offsets[12]={202216,219648,355336,355416,355848,367208,475464,485584,525032,540640,541800,544224};
    if(MeleePc_DatBe32(bytes)!=550322 || body!=544260 || relocs!=1393 ||
       publics!=12 || MeleePc_DatBe32(bytes+16)!=0)return 0;
    for(unsigned i=0;i<12;++i)
        if(MeleePc_DatBe32(bytes+roots+i*8)!=offsets[i] ||
           strcmp((char*)bytes+strings+MeleePc_DatBe32(bytes+roots+i*8+4),names[i]))return 0;
    return 1;
}
#endif
