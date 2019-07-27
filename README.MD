# bayan - utility for finding duplicate files

## Parameters:

- -d --dirs - directory for scanning. Default is current dir
- -e --exclude - directories for excluding (could be a few). Default is empty parameter
- -l --level - scanning level (0 - scan internal directories, 1 - scan all nested directories). Default is 1
- -s --size - minimum size of file. Default is 1 byte
- -m --mask - mask for filenames. Default is empty mask. *Important:* You need to use ' ' for mask symbols **\***, **\.** and **?**
- -b --blocksize - block size for reading. Default is 5
- -a --algorithm - hash algorithm (crc32, md5). Default is crc32

**Output:** duplicate files list with pull paths, divided by empty lines