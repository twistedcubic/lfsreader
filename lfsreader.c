#include "lfsreader.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BSIZE 4096
void ls(inode *);
void cat(inode *);
inode *getFile(inode*);
char *path[20]; //path to file/directory
void *fsptr; //start of fs
inode *getI(ushort inum); //get address of nth inode
checkpoint *cp;
int nArg; //next arg in path, used in recursion
inode *root; //root has inode number 0
int pathC = 0; //# of tokens in path
int pathInum; //inum to file/directory given by path

int main(int argc, char **argv)
{
  //  void *fsptr;
  int fsize;
  struct stat buf;

  //read in file image
  int fd = open(argv[3], O_RDWR);
  //  printf("%d\n", fd);
  //   printf("%s\n", strerror(errno));
  fstat(fd, &buf);
  fsize = buf.st_size;
  fsptr = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  //  printf("%s\n", strerror(errno));
  cp = (checkpoint*)fsptr; //checkpoint
  //  printf("Checkpoint size:%d \n", cp->size);
  //  printf("Checkpoint start:%p \n", fsptr);
  root = (inode*)(fsptr+((inodeMap*)(fsptr+cp->iMapPtr[0]))->inodePtr[0]); //pointer to inode 0
  //  printf("root->size:%d\n",root->size);
   
  //splits absolute path into tokens using deliminater "/"
  //  printf("%s\n", argv[2]);
  path[0] = strtok(argv[2], "/");
  pathC = 0; //count extra null at the end!
  int i;
  for(i = 1; path[i-1]!=NULL; i++) //null terminated
    {
      path[i] = strtok(NULL, "/");
      //      printf("%s \n", path[i]);
      pathC++;
    }
  nArg = 0;
  
  inode *file;
  if(path[0] == 0)
    file = root;
  else
    file = getFile(root); //desired file at path
  
  if(strcmp(argv[1], "ls") == 0)
     ls(file);
    
  if(strcmp(argv[1], "cat")==0 )
     cat(file);
    
  return 0;
}
//get address of nth inode
inode *getI(ushort inum){
  //  return (inode*)(fsptr + ((inodeMap*)(fsptr+cp->iMapPtr[inum/16]))->inodePtr[inum%16]);  //16 is number of entries in iMap
  return (inode*)(fsptr + ((inodeMap*)(fsptr+cp->iMapPtr[inum/16]))->inodePtr[inum%16]);  //16 is number of entries in iMa
}

//get inode address of file/directory pointed to by path
//recursively; set int nArg = 0 before calling
inode *getFile(inode* in){ //input pointer to inode
  //  inode *tempI;
  dirEnt *tempDe;
  int found = 0;
  if(in->type == 1) //is file
    {
      if(path[nArg+1] != NULL)  //check if file but not end of path, shouldn't happen
      //	printf("Err_or: file in middle of path\n");
	return in; //this inode pointed to a file
    }else if(in->type == 0) //is directory
    {  //iterate through dirEnts pointed to by inode, 14 total
      int i;
      for(i = 0; i<14; i++) //multiple dirents/block
	{  
	  if(in->ptr[i] < 0) //invalid entry and no match found
	    { printf("Error!\n");
	      exit(0);
	    }
	  //check type first! whether directory or file      
	  tempDe = (dirEnt*)(fsptr + in->ptr[i]);
	  for( ; (tempDe->name[0]) != 0; tempDe++)
	    {
	      if(strcmp(tempDe->name, path[nArg])==0)
		{found = 1;
		  break;
		}
	    }
	  if(found == 1) //found match
	    break;
	}
      if(path[nArg+1] == NULL) //reached end of path
	{pathInum = tempDe->inum;
	  return getI(tempDe->inum);
	}
      else if(path[nArg+1] != 0) //not at end of path yet
	{
	  nArg++;
	  return getFile(getI(tempDe->inum));
	}
    }
  return NULL; //shouldn't reach here
}
//where is file name located? -- in directory entry field "pointing" to this inode

//print out the content of the directory given its inode address
void ls(inode *in){
   if(in->type != 0)
     {printf("Error!\n");
       return;
     }
  int i;
  dirEnt *tempDe;
  for(i=0; i<14 && in->ptr[i] > 0; i++) //list of pointers in inode all point to directory entries
    {
      tempDe = (dirEnt*)(fsptr + in->ptr[i]);
      for( ; (tempDe->name[0]) != 0; tempDe++)
	{      //ptr list could skip entries
      	  printf("%s", tempDe->name);
	  //print "/" at end if directory
	  if(getI(tempDe->inum)->type == 0) //type is directory
	    printf("/\n");
	  else
	    printf("\n");
	}
    }
  //  return 0;
}
//print out the content of file given its inode address
void cat(inode *in){
  //make sure it's a file
  if(in->type != 1)
    { printf("Error!\n");
      return;
    }
  int sz = in->size;
  write(1, fsptr + in->ptr[0], sz); //assume ptr[0] is start of file
  //go to other blocks for addrs in ptr[i]
  //  return 0;
}
