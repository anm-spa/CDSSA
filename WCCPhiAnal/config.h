//
//  utils.h
//  LLVM
//
//  Created by Abu Naser Masud on 2022-02-17.
//

#ifndef config_h
#define config_h
namespace{

void SplitFilename (const string& str, string &path, string &filename)
{
    size_t found;
    found=str.find_last_of("/\\");
    path=str.substr(0,found);
    filename=str.substr(found+1);
}

static unsigned debugLevel;

}

#endif /* config_h */
