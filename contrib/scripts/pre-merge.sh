#!/bin/bash

mkdir tmpmerge
cp src/chainparamsbase.cpp tmpmerge/chainparamsbase.cpp
cp src/chainparams.cpp tmpmerge/chainparams.cpp
cp src/chainparams.h tmpmerge/chainparams.h
cp src/blockrewards.h tmpmerge/blockrewards.h
cp src/masternodeconfig.cpp tmpmerge/masternodeconfig.cpp
cp src/masternode.cpp tmpmerge/masternode.cpp
cp src/netbase_tests.cpp tmpmerge/netbase_tests.cpp
cp src/masternodeman.cpp tmpmerge/masternodeman.cpp
cp Dockerfile tmpmerge/Dockerfile

