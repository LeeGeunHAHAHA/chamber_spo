#!/bin/bash

PATH_FQA=~/fqa/
PATH_FQA_TRUNK=$PATH_FQA/trunk
PATH_QUAL=~/qual/
GREEN='\033[1;32m'
RED='\033[1;31m'
NC='\033[0m'






echo -e "${RED}[${GREEN}${PATH_QUAL_TRUNK}${RED}]${NC}"
cd $PATH_QUAL

echo -e "${RED}[${GREEN}svn up${RED}]${NC}"
svn up --no-auth-cache





echo -e "${RED}[${GREEN}${PATH_FQA_TRUNK}${RED}]${NC}"
cd $PATH_FQA

echo -e "${RED}[${GREEN}svn up${RED}]${NC}"
svn up --no-auth-cache


cd $PATH_FQA_TRUNK
echo -e "${RED}[${GREEN}make clean${RED}]${NC}"
make clean

echo -e "${RED}[${GREEN}make native${RED}]${NC}"
make native
