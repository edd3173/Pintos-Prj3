#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "lib/kernel/console.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/syscall.h"
#include "userprog/process.h"

#define MAX_FD_NUM 128

/*
void getFirstToken(char* IN, char* OUT){
     int i=0;
     int LEN=strlen(IN);
     strlcpy(OUT,IN,LEN+1);
	 while(OUT[i]!='\0' && OUT[i]!= ' '){
         i++;
     }
     OUT[i]='\0';
     return;
 }
*/


struct lock FILELOCK;

struct file{
	struct inode* inode;
	off_t pos;
	bool deny_write;
};

static void syscall_handler (struct intr_frame *);

void 
halt(void){
	//printf("Entered halt function\n");
	shutdown_power_off();
}


void 
exit(int status){
	//struct thread* curr=thread_current();
	printf("%s: exit(%d)\n",thread_name(), status);	
	thread_current()->exit_status=status;
	for(int i=3;i<MAX_FD_NUM;i++){
		if(thread_current()->FD[i]!=NULL)
			close(i);
	}

	struct thread* T=NULL;
	struct list_elem* E=NULL;


 	E=list_begin(&thread_current()->child);
  	while(E!=list_end(&thread_current()->child)){
    	T=list_entry(E, struct thread, child_element);
    	process_wait(T->tid);
    	E=list_next(E);
  	}





	thread_exit();
}

pid_t 
exec(const char* cmd){
	
	char CMD[64];
	getFirstToken(cmd,CMD);
	
	struct file* fPtr=filesys_open(CMD);
	if(fPtr==NULL)
		return -1;

	//file_close(fPtr);

	return process_execute(cmd);
}

int 
wait(pid_t pid){
	return process_wait(pid);
}




void
check_address(void* addr){
  if(!is_user_vaddr(addr)){
    //printf("Invalid address!\n");
    exit(-1);    
  }
}


int 
write (int fd, const void* buffer, unsigned size){
  int res=-1;
  check_address(buffer);
  lock_acquire(&FILELOCK);
  if (fd == 1) {
    putbuf(buffer, size);
    res= size;
  } 
  else if (fd >= 3) {
	if(thread_current()->FD[fd]==NULL){
		lock_release(&FILELOCK);
		exit(-1);
	}
    if(thread_current()->FD[fd]->deny_write){
		file_deny_write(thread_current()->FD[fd]);
	}
	res = file_write(thread_current()->FD[fd], buffer, size);
  }
  lock_release(&FILELOCK);
  return res;
}

int 
read(int fd, void* buffer, unsigned size){
	int res=-1;
	unsigned int i;
	check_address(buffer);
	lock_acquire(&FILELOCK);
	if(fd==0){
		for(i=0;i<size;i++){
			if(input_getc()=='\0')
				break;
		}
		res=i;
	}
	else if(fd >= 3){
		if(thread_current()->FD[fd]==NULL){
			lock_release(&FILELOCK);
			exit(-1);
		}
		res = file_read(thread_current()->FD[fd],buffer,size);
	}
	lock_release(&FILELOCK);
	return res;
}

bool 
create (const char *file, unsigned initial_size) {
	bool result;
	if(file==NULL)
		exit(-1);
	check_address(file);
	
	result = filesys_create(file, initial_size);
	return result;
}

bool 
remove (const char *file) {
	bool result;
	if(file==NULL)
		exit(-1);
	check_address(file);
	result = filesys_remove(file);
	return result;
}


int 
open (const char *file) {
  int res=-1;
  if(file==NULL)
	  exit(-1);
  check_address(file);
  lock_acquire(&FILELOCK);
  struct file* fPtr = filesys_open(file);
  if (fPtr == NULL) {
      res = -1; 
  } else {
		for (int i = 3; i < MAX_FD_NUM; i++) {
			if(strcmp(thread_current()->name,file)==0 && thread_current()->FD[i]==NULL){
				file_deny_write(fPtr);
			}
			if (thread_current()->FD[i] == NULL) {
				thread_current()->FD[i] = fPtr; 
				res= i;
				break;
			}	   
		}	   
  }
  lock_release(&FILELOCK);
  return res; 
}

int 
filesize (int fd) {
  int res=-1;
  if(thread_current()->FD[fd]==NULL)
	  exit(-1);
  res = file_length(thread_current()->FD[fd]);
  return res;
}


void 
seek (int fd, unsigned position) {
  if (thread_current()->FD[fd] == NULL) {
    exit(-1);
  }
  file_seek(thread_current()->FD[fd], position);
}

unsigned 
tell (int fd) {

  if (thread_current()->FD[fd] == NULL) 
    exit(-1);
  unsigned res = (unsigned)file_tell(thread_current()->FD[fd]);
  return res;
  //return (unsigned)file_tell(thread_current()->FD[fd]);
}

void close (int fd) {
  if (thread_current()->FD[fd] == NULL) {
	  exit(-1);   
  }

  struct file* fPtr=thread_current()->FD[fd];
  thread_current()->FD[fd]=NULL;
  file_close(fPtr);
}





int 
fibonacci(int n){
	//printf("Entered Fibo! %d\n",n);
	if(n==0)
		return 0;
	else{
		int a=0;
		int b=1;
		int cnt=2;
		while(cnt<n){
			int c=b;
			b=a+b;
			a=c;
			++cnt;
		}
		return a+b;
	}
}

int 
max_of_four_int(int a,int b,int c,int d){
	//printf("Entered MOF! %d %d %d %d\n",a,b,c,d);
	int arr[4];
	arr[0]=a; arr[1]=b; arr[2]=c; arr[3]=d;
	int max=arr[0];
	for(int i=0;i<4;i++){
		if(arr[i]>=max){
			max=arr[i];
		}
	}
	return max;
}




void
syscall_init (void) 
{
  lock_init(&FILELOCK);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f ) 
{
  //printf ("system call! : %d\n",*(uint32_t*)(f->esp));
  //hex_dump(f->esp, f->esp, 100, 1);
  void* ESP=f->esp;

  switch(*(uint32_t*)(ESP)){
	  case SYS_HALT:
		  //printf("SYS_HALT!\n");
		  halt();
		  break;

	  case SYS_EXIT:

		  check_address(ESP+4);
		  exit(*(uint32_t *)(ESP+4));
		  break;

	  case SYS_EXEC:
		  
		  check_address(ESP+4); 
		  f->eax=exec((const char*)*(uint32_t*)(ESP+4));
		  break;

	  case SYS_WAIT:
		  
		  check_address(ESP+4);
		  f->eax=wait((pid_t)*(uint32_t*)(ESP+4));
		  break;

	  case SYS_READ:
		  
		  check_address(ESP+4);
		  check_address(ESP+8);
		  check_address(ESP+12);
		  f->eax = read ( (int)*(uint32_t *)(ESP + 4), (void *)*(uint32_t *)(ESP + 8), (unsigned)*((uint32_t *)(ESP + 12)) );
		  break;

	  case SYS_WRITE:
		  
		  check_address(ESP+4);
		  check_address(ESP+8);
		  check_address(ESP+12);
		  f->eax = write((int)*(uint32_t*)(ESP+4),(void*)*(uint32_t*)(ESP+8),(unsigned)*((uint32_t*)(ESP+12)));
		  break;

		  
	  case SYS_CREATE:
		  
		  check_address(ESP + 4);
		  check_address(ESP + 8);
		  f->eax = create((const char *)*(uint32_t *)(ESP + 4), (unsigned)*(uint32_t *)(ESP + 8));
		  break;
    
	  case SYS_REMOVE:
      
		  check_address(ESP + 4);
		  f->eax = remove((const char*)*(uint32_t *)(ESP + 4));
		  break;
    
	  case SYS_OPEN:
		  
		  check_address(ESP + 4);
		  f->eax = open((const char*)*(uint32_t *)(ESP + 4));
		  break;
    
	  case SYS_FILESIZE:
      
		  check_address(ESP + 4);
		  f->eax = filesize((int)*(uint32_t *)(ESP + 4));
		  break;
    
	  case SYS_SEEK:
      
		  check_address(ESP + 4);
		  check_address(ESP + 8);
		  seek((int)*(uint32_t *)(ESP + 4), (unsigned)*(uint32_t *)(ESP + 8));
		  break;
    
	  case SYS_TELL:
      
		  check_address(ESP + 4);
		  f->eax = tell((int)*(uint32_t *)(ESP + 4));
		  break;
    
	  case SYS_CLOSE:
      
		  check_address(ESP + 4);
		  close((int)*(uint32_t *)(ESP + 4));
		  break;



	  case SYS_FIBONACCI:
		  check_address(ESP+4);
		  f->eax = fibonacci((int)*(uint32_t*)(ESP+4));
		  break;

	  case SYS_MAX_OF_FOUR_INT:
		  check_address(ESP+4);
		  check_address(ESP+8);
		  check_address(ESP+12);
		  check_address(ESP+16);
		  f->eax=max_of_four_int((int)*(uint32_t*)(ESP+4),(int)*(uint32_t*)(ESP+8),(int)*(uint32_t*)(ESP+12),(int)*(uint32_t*)(ESP+16));
		  break;
  }
  //thread_exit ();
}
