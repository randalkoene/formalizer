// Copyright 2020 Randal A. Koene
// License TBD

/**
 * General collection of functions used for binary I/O with files and streams.
 */

// std
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

// core
#include "error.hpp"
#include "binaryio.hpp"

namespace fz {

char * read_file_into_buf(std::string file_path, size_t & lSize) {
    FILE *pFile;
    char *buffer;
    size_t result;

    pFile = fopen(file_path.c_str(), "rb");
    if (pFile == NULL) {
        fputs("File error", stderr);
        exit(1);
    }

    // obtain file size:
    fseek(pFile, 0, SEEK_END);
    lSize = ftell(pFile);
    rewind(pFile);

    // allocate memory to contain the whole file:
    buffer = (char *)malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fputs("Memory error", stderr);
        exit(2);
    }

    // copy the file into the buffer:
    result = fread(buffer, 1, lSize, pFile);
    if (result != lSize) {
        fputs("Reading error", stderr);
        exit(3);
    }

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose(pFile);
    return buffer;
}

/**
 * Read the full contents of a binary file into a buffer.
 * 
 * @param path of the file.
 * @param buf reference to the receiving buffer.
 * @param readstate returns the iostate flags when provided (default: nullptr)
 * @return true if the read into buffer was successful.
 */
bool file_to_buffer(std::string path, std::vector<char> & buf, std::ifstream::iostate * readstate) {
    std::ifstream ifs(path, std::ifstream::in | std::ios::binary);

    if (readstate) (*readstate) = ifs.rdstate();

    if (ifs.fail())
        ERRRETURNFALSE(__func__,"unable to open file "+path);

    ifs.seekg(0, std::ios::end);
    buf.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);

    buf.assign((std::istreambuf_iterator<char>(ifs)),std::istreambuf_iterator<char>());
    ifs.close();
    return true;
/*
    size_t lSize;
    char * buffer = read_file_into_buf(path, lSize);

    FZOUT("GOT BACK FROM LOADING "+std::to_string(lSize)+" bytes\n"); base.out->flush();

    buf.resize(lSize);
    for (size_t i = 0; i < lSize; ++i) {
        buf[i].c = buffer[i];
    }
    free(buffer);
    FZOUT("DID COPY THE BUFFER\n"); base.out->flush();
    return true;
*/
/*
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);

    if (readstate) (*readstate) = ifs.rdstate();

    if (ifs.fail())
        ERRRETURNFALSE(__func__,"unable to open file "+path);

    std::streamsize ifs_size = ifs.tellg();
    //FZOUT("IFS_SIZE: "+std::to_string(ifs_size)+'\n'); base.out->flush();
    ifs.seekg(0, std::ios::beg);

    buf.resize(ifs_size); // be careful to use resize() here and not reserve() so that size() makes sense.
    FZOUT("buffer size: "+std::to_string(buf.size())+'\n'); base.out->flush();
    char cbuf[100000];
    bool res = ifs.read(cbuf, ifs_size).good();
    FZOUT("RETURNED FROM READ\n"); base.out->flush();
    memcpy(cbuf,buf.data(),ifs_size);
    FZOUT("CAME BACK FROM READ\n"); base.out->flush();
    return res; */
}

} // namespace fz
