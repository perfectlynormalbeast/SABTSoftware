/**
 * @file FAT32.h
 * @brief FAT32 implementation of SD card
 * ********************************************************
 * **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD *****
 * ********************************************************
 * Controller: ATmega32 (Clock: 8 Mhz-internal)
 * Compiler  : AVR-GCC (winAVR with AVRStudio)
 * Version   : 2.3
 * Author  : CC Dharmani, Chennai (India)
 *         www.dharmanitech.com
 * Date    : 08 May 2010
 * ********************************************************
 * @author CC Dharmani, Chennai (India)
 * @author Nick LaGrow (nlagrow)
 * @author Alex Etling (petling)
 * @author Kory Stiger (kstiger)
 *
 * Link to the Post: 
 *  http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html
 */

#ifndef _FAT32_H_
#define _FAT32_H_

//buffer variable
#define BUFFER_SIZE      512
//BOARD WILL RESET IF NOT SET TO 13
#define FILE_NAME_LEN    13
#define END_OF_FILE      26
#define CLUSTERS_PER_RUN 60
#define MAX_NUM_CLUSTERS 512  //max number of clusters that can be in teh dictionary file you are using 
                              //MAKE SURE TO ABIDE BY IT

//Attribute definitions for file/directory
#define ATTR_READ_ONLY     0x01
#define ATTR_HIDDEN        0x02
#define ATTR_SYSTEM        0x04
#define ATTR_VOLUME_ID     0x08
#define ATTR_DIRECTORY     0x10
#define ATTR_ARCHIVE       0x20
#define ATTR_LONG_NAME     0x0f

#define DIR_ENTRY_SIZE     0x32
#define EMPTY              0x00
#define DELETED            0xe5
#define GET                0
#define SET                1
#define READ               0
#define VERIFY             1
#define ADD                0
#define REMOVE             1
#define LOW                0
#define HIGH               1
#define TOTAL_FREE         1
#define NEXT_FREE          2
#define GET_LIST           0
#define GET_FILE           1
#define DELETE             2
#define FAT32_EOF                0x0fffffff

// Structure to access Master Boot Record for getting info about partioions
struct MBRinfo_Structure
{
  unsigned char nothing[446];           // ignore, fill the gap in the structure
  unsigned char partition_data[64];     // partition records (16x4)
  unsigned int signature;               // 0xaa55
};

// Structure to access info of the first partioion of the disk 
struct partitionInfo_Structure
{         
  unsigned char status;                 // 0x80 - active partition
  unsigned char head_start;             // starting head
  unsigned int cyl_sect_start;          // starting cylinder and sector
  unsigned char type;                   // partition type
  unsigned char head_end;               // ending head of the partition
  unsigned int cyl_sect_end;            // ending cylinder and sector
  // total sectors between MBR & the first sector of the partition
  unsigned long first_sector;    
  unsigned long sectors_total;          // size of this partition in sectors
};

//Structure to access boot sector data
struct BS_Structure
{
  unsigned char jump_boot[3];           // default: 0x009000EB
  unsigned char oem_name[8];
  unsigned int bytes_per_sector;        // default: 512
  unsigned char sector_per_cluster;
  unsigned int reserved_sector_count;
  unsigned char number_of_fats;
  unsigned int root_entry_count;
  unsigned int total_sectors_f16;       // must be 0 for FAT32
  unsigned char media_type;
  unsigned int fat_size_f16;            // must be 0 for FAT32
  unsigned int sectors_per_track;
  unsigned int number_of_heads;
  unsigned long hidden_sectors;
  unsigned long total_sectors_f32;
  unsigned long fat_size_f32;           // count of sectors occupied by one FAT
  unsigned int ext_flags;
  unsigned int fs_version;              // 0x0000 (defines version 0.0)
  unsigned long root_cluster;           // first cluster of root directory (=2)
  unsigned int fs_info;                 // sector number of FSinfo structure (=1)
  unsigned int backup_boot_sector;
  unsigned char reserved[12];
  unsigned char drive_number;
  unsigned char reserved1;
  unsigned char boot_signature;
  unsigned long volume_id;
  unsigned char volume_label[11];       // "NO NAME"
  unsigned char file_system_type[8];    // "FAT32"
  unsigned char boot_data[420];
  unsigned int boot_end_signature;      // 0xaa55
};


//Structure to access FSinfo sector data
struct FSInfo_Structure
{
  unsigned long lead_signature;         // 0x41615252
  unsigned char reserved1[480];
  unsigned long structure_signature;    // 0x61417272
  unsigned long free_cluster_count;     // initial: 0xffffffff
  unsigned long next_free_cluster;      // initial: 0xffffffff
  unsigned char reserved2[12];
  unsigned long trail_signature;        // 0xaa550000
};

//Structure to access Directory Entry in the FAT
struct dir_Structure
{
  unsigned char name[11];
  unsigned char attrib;                 // file attributes
  unsigned char nt_reserved;            // always 0
  unsigned char time_tenth;             // tenths of seconds, set to 0 here
  unsigned int create_time;             // time file was created
  unsigned int create_date;             // date file was created
  unsigned int last_access_date;
  unsigned int first_cluster_hi;        // higher word of the first cluster number
  unsigned int write_time;              // time of last write
  unsigned int write_date;              // date of last write
  unsigned int first_cluster_lo;        // lower word of the first cluster number
  unsigned long file_size;              // size of file in bytes
};


//************* external variables *************
volatile unsigned long first_data_sector, root_cluster, total_clusters;
volatile unsigned int  bytes_per_sector, sector_per_cluster, reserved_sector_count;
unsigned long unused_sectors, append_file_sector;
unsigned long append_file_location, file_size, append_start_cluster;

// Global flag to keep track of free cluster count updating in FSinfo sector
unsigned char free_cluster_count_updated;

//for Text files track all clusters that they contain
bool done_rd_dict;
unsigned long curr_cluster;
//global to track where we are in reading in initial dictionary file
unsigned long curr_dict_cluster;
//location of the file - do not need to look it up anymore once you have it
struct dir_Structure *dict_dir;
unsigned long *dict_clusters;
//will be set to 1 if there is a preceeding word overlapping into this cluster, 0 if this cluster starts
//word
unsigned char *preceeding_word;
//global to track total number clusters read in 
unsigned int dict_cluster_cnt;

//************* functions *************
unsigned char convert_dict_file_name (unsigned char *file_name);
bool find_word_in_cluster(unsigned char *word, unsigned long arr_cluster_index);
unsigned char init_read_dict(unsigned char *file_name);
bool find_wrd_in_buff(unsigned char *word);
unsigned char read_dict_file();
bool bin_srch_dict(unsigned char *word);
int check_first_full_word(unsigned char *word, char overlap);
unsigned char get_boot_sector_data(void);
unsigned long get_first_sector(unsigned long cluster_number);
unsigned long get_set_free_cluster(unsigned char tot_or_next, 
                                   unsigned char get_set, 
                                   unsigned long fs_entry);
struct dir_Structure* find_files(unsigned char flag, unsigned char *file_name);
unsigned long get_set_next_cluster(unsigned long cluster_number,
                                   unsigned char get_set,
                                   unsigned long cluster_entry);
unsigned char read_file(unsigned char flag, unsigned char *file_name);
unsigned char read_and_retrieve_file_contents(unsigned char *file_name,
                                              unsigned char *data_string);
unsigned char play_mp3_file(unsigned char *file_name);
unsigned char play_beep();
unsigned char convert_file_name(unsigned char *file_name);
int replace_the_contents_of_this_file_with(unsigned char *file_name,
                                           unsigned char *file_content);
void write_file(unsigned char *file_name);
void append_file(void);
unsigned long search_next_free_cluster(unsigned long start_cluster);
void memory_statistics(void);
void display_memory(unsigned char flag, unsigned long memory);
void delete_file(unsigned char *file_name);
void free_memory_update(unsigned char flag, unsigned long size);
void init_sd_card(bool verb);


#endif /* _FAT32_H_ */
