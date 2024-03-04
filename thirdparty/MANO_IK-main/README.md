# MANO_IK
MANO hand model and IK optimize process interface

MANO implement based on https://github.com/Smorodov/nano_smpl

AIK based on https://github.com/MengHao666/Minimal-Hand-pytorch (python code)

## Prerequisites(All included in this repository)
---
Eigen

zlib

## Compile && run
---

    mkdir build
    cd build && make -j8
    ./nano_smpl

## Usage
---

First you should define a hand model object like:
    
    #include "mano_refine.h"
    ManoOptimizer mano_opt("../scripts/model/MANO_RIGHT.npz", 1);

The second param 0 means infering as left hand model and 1 means right hand mode.

MANO model interface :
    
    j3d_refined = mano_opt.forward(j3d_pre);

## Model preprocessing
---
You need to preprocess initial pkl model format to npz using script from subfolder scripts/preprocess.py and copy result npz file to exe's folder to model subfolder.

