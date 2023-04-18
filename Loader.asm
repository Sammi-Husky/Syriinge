#################################################
Don't clear memory on game launch [Sammi Husky]
#################################################
op nop @ $801d5f64
op nop @ $801d5f68

#######################################################
run codehandler after initializing memory [Sammi Husky]
#######################################################
HOOK @ $80016c30 
{
    # __memfill
    lis r12, 0x8000
    ori r12, r12, 0x443c
    mtctr r12
    bctrl
    
    # codehandler
    lis r12, 0x8000
    ori r12, r12, 0x18a8
    mtctr r12
    bctrl
    
}

##################################################
run codehandler after loading rels [Sammi Husky]
##################################################
hook @ $800272b8
{
    stwu r1, -0x80(r1)
    mflr r0
    stw r0, 0x84(r1)
    stmw r3, 8(r1)
    
    # codehandler
    lis r12, 0x8000
    ori r12, r12, 0x18a8
    mtctr r12
    bctrl
    
    lmw r3, 8(r1)
    lwz r0, 0x84(r1)
    mtlr r0
    addi r1, r1, 0x80
    
    lwz r4, 0x0(r30)
}



########################################
Syringe Core Loader [Sammi Husky]
########################################
word 0x79b00 @ $80421D84
word 0x4Cb900 @ $80422254
word 0x393400 @ $80422394
HOOK @ $80ad6738
{
    stwu r1, -0x20(r1)
    bl _main
    # for some reason GCTRM wont assemble this string
    word 0x53797269 # "Syringe"
    word 0x6E676500
    
    word 0x73795F63 # "sy_core.rel"
    word 0x6F72652E
    word 0x72656C00

_main:
    mflr r31
    
    # check if we have BrawlEx. 
    # we do this because if BEX is running
    # our heap addr will be different
    li r3, 0x3B
    lis r12, 0x8002
    ori r12, r12, 0x49cc
    mtctr r12
    bctrl # getHeap
    
    cmpwi r3, 0
    beq _noBex
_hasBex:
    lis r3, 0x817A
    b _createHeap
_noBex:
    lis r3, 0x817B
    
_createHeap:
    ori r3, r3, 0xa5a0
    lis r4, 2
    mr r5, r31
    lis r6, 0x8049
    ori r6, r6, 0x4D18
    stw r5, 0(r6)
    stw r3, 4(r6)
    stw r4, 8(r6)
    lis r12, 0x8002
    ori r12, r12, 0x59A4
    mtctr r12
    bctrl
    

    addi r3, r1, 0x8
    lwz r4, -0x43e8(r13)
    addi r5, r31, 8
    li r6, 0x3C
    li r7, 1
    li r8, 0
    lis r12, 0x8002
    ori r12, r12, 0x6E00
    mtctr r12
    bctrl #loadModuleRequest

    addi r1, r1, 0x20
    lwz r0, 0x14(r1) # original instruction
}