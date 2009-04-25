/* 
 * VMFS volumes.
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "vmfs.h"
#include "scsi.h"

/* Read a data block from the physical volume */
static ssize_t vmfs_vol_read_data(vmfs_volume_t *vol,off_t pos,
                                  u_char *buf,size_t len)
{
   if (fseeko(vol->fd,pos,SEEK_SET) != 0)
      return(-1);

   return(fread(buf,1,len,vol->fd));
}

/* Read a raw block of data on logical volume */
ssize_t vmfs_vol_read(vmfs_volume_t *vol,off_t pos,u_char *buf,size_t len)
{
   pos += vol->vmfs_base + 0x1000000;

   return(vmfs_vol_read_data(vol,pos,buf,len));
}

/* Write a data block to the physical volume */
static ssize_t vmfs_vol_write_data(vmfs_volume_t *vol,off_t pos,
                                   u_char *buf,size_t len)
{
   if (fseeko(vol->fd,pos,SEEK_SET) != 0)
      return(-1);

   return(fwrite(buf,1,len,vol->fd));
}

/* Write a raw block of data on logical volume */
ssize_t vmfs_vol_write(vmfs_volume_t *vol,off_t pos,u_char *buf,size_t len)
{
   pos += vol->vmfs_base + 0x1000000;

   return(vmfs_vol_write_data(vol,pos,buf,len));
}

/* Read volume information */
static int vmfs_volinfo_read(vmfs_volinfo_t *vol,FILE *fd)
{
   u_char buf[1024];

   if (fseeko(fd,VMFS_VOLINFO_BASE,SEEK_SET) != 0)
      return(-1);

   if (fread(buf,sizeof(buf),1,fd) != 1)
      return(-1);

   vol->magic = read_le32(buf,VMFS_VOLINFO_OFS_MAGIC);

   if (vol->magic != VMFS_VOLINFO_MAGIC) {
      fprintf(stderr,"VMFS VolInfo: invalid magic number 0x%8.8x\n",
              vol->magic);
      return(-1);
   }

   vol->version = read_le32(buf,VMFS_VOLINFO_OFS_VER);
   vol->lun = buf[VMFS_VOLINFO_OFS_LUN];

   vol->name = strndup((char *)buf+VMFS_VOLINFO_OFS_NAME,
                       VMFS_VOLINFO_OFS_NAME_SIZE);

   memcpy(vol->uuid,buf+VMFS_VOLINFO_OFS_UUID,sizeof(vol->uuid));

   vol->size    = read_le64(buf,VMFS_LVMINFO_OFS_SIZE);
   vol->blocks  = read_le64(buf,VMFS_LVMINFO_OFS_BLKS);
   vol->num_segments = read_le32(buf,VMFS_LVMINFO_OFS_NUM_SEGMENTS);
   vol->first_segment = read_le32(buf,VMFS_LVMINFO_OFS_FIRST_SEGMENT);
   vol->last_segment = read_le32(buf,VMFS_LVMINFO_OFS_LAST_SEGMENT);
   vol->num_extents = read_le32(buf,VMFS_LVMINFO_OFS_NUM_EXTENTS);

   memcpy(vol->lvm_uuid,buf+VMFS_LVMINFO_OFS_UUID,sizeof(vol->lvm_uuid));

   return(0);
}

/* Show volume information */
void vmfs_volinfo_show(vmfs_volinfo_t *vol)
{
   char uuid_str[M_UUID_BUFLEN];

   printf("Physical Volume Information:\n");
   printf("  - UUID    : %s\n",m_uuid_to_str(vol->uuid,uuid_str));
   printf("  - Version : %d\n",vol->version);
   printf("  - Name    : %s\n",vol->name);
   printf("  - Num. Segments : %u\n",vol->num_segments);
   printf("  - First Segment : %u\n",vol->first_segment);
   printf("  - Last Segment  : %u\n",vol->last_segment);

   printf("\n");
}

/* Create a volume structure */
vmfs_volume_t *vmfs_vol_create(char *filename,int debug_level)
{
   vmfs_volume_t *vol;
   struct stat st;

   if (!(vol = calloc(1,sizeof(*vol))))
      return NULL;

   if (!(vol->filename = strdup(filename)))
      goto err_filename;

   if (!(vol->fd = fopen(vol->filename,"r"))) {
      perror("fopen");
      goto err_open;
   }

   vol->debug_level = debug_level;
   fstat(fileno(vol->fd),&st);
   vol->is_blkdev=S_ISBLK(st.st_mode);
   return vol;

 err_open:
   free(vol->filename);
 err_filename:
   free(vol);
   return NULL;
}

/* Open a VMFS volume */
int vmfs_vol_open(vmfs_volume_t *vol)
{
   vol->vmfs_base = VMFS_VOLINFO_BASE;

   /* Read volume information */
   if (vmfs_volinfo_read(&vol->vol_info,vol->fd) == -1) {
      fprintf(stderr,"VMFS: Unable to read volume information\n");
      return(-1);
   }

   if (vol->is_blkdev && (scsi_get_lun(fileno(vol->fd)) != vol->vol_info.lun))
      fprintf(stderr,"VMFS: Warning: Lun ID mismatch on %s\n", vol->filename);

   if (vol->debug_level > 0) {
      vmfs_volinfo_show(&vol->vol_info);
      printf("VMFS: volume opened successfully\n");
   }
   return(0);
}
