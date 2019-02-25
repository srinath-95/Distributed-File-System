#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<math.h>
#include<signal.h>
#include<sys/stat.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include <openssl/md5.h>

#define BUF_SIZE 2048 

int cli_sock[4] = {0};
int ipassword_match[4] ={0};
int x = 0;
char key;
struct sockaddr_in dfs_address[4];

char command[30];
char filebuffer[BUF_SIZE];
char file_name[20];
int a[8] ={0};


typedef enum{
	ERROR = -1,
	SUCCESS = 1,
}status;

typedef struct{
	char name [4][20];
	char server [4][20];
	char port [4][10];
	char username [50];
	char pwd [50];

}dfc_file;

typedef struct{
	char command_choice[8];
	char filename[20];
	char username[50];
	int pieces;
	int packetcount;
	long int file_piece_length;
	char subfolder[20];
}packet;

packet packet_info;
dfc_file dfc;

typedef struct{
        long int seq_no;
        char buffer[BUF_SIZE];
        long int frame_size;
}frame_t;

typedef struct{
	int packetcount;
	char filename[50];
}get_t;

get_t get_info;

frame_t frame;

void LUT(int x)
{
	printf("\n Entered the LUT function");
	printf("\n The x value is: %d",x);

	
	if (x == 0)
	{
		a[0] =  0 ;  a[1] =  1 ;
		a[2] =  1 ;  a[3] =  2 ;
		a[4] =  2 ;  a[5] =  3 ;
		a[6] =  3 ;  a[7] =  0 ;
	}
	else if (x == 1)
	{
		a[0] =  2 ;  a[1] =  3 ;
		a[2] =  3 ;  a[3] =  0 ;
		a[4] =  0 ;  a[5] =  1 ;
		a[6] =  2 ;  a[7] =  3 ;
	}
	else if (x == 2)
	{
		a[0] =  0 ;  a[1] =  1 ;
		a[2] =  1 ;  a[3] =  2 ;
		a[4] =  2 ;  a[5] =  3 ;
		a[6] =  3 ;  a[7] =  0 ;
	}
	else
	{
		a[0] =  1 ;  a[1] =  2 ;
		a[2] =  2 ;  a[3] =  3 ;
		a[4] =  3 ;  a[5] =  0 ;
		a[6] =  0 ;  a[7] =  1 ;
	}

}


/* Validation in the Config file*/
status config_file(char *file_name)
{
	printf("\n --------Entered the function to open config file");
	printf("\n The entered file name is: %s",file_name);
	FILE *f;

	f = fopen(file_name,"r");
	if(f != NULL)
	{
		char config_file_buffer[500];
		char config_file_cmp[30];
		char *ptr = NULL;
		//char c;
		int fil_size_check =0;
		printf("\n File has been opened");
		memset(config_file_buffer,0,500);
		
		fseek(f,0,SEEK_END);
        fil_size_check = ftell(f);
        printf("\n file_size_check: %d",fil_size_check);
        fseek(f,0,SEEK_SET);
		fread(config_file_buffer,1,fil_size_check,f);	
		fclose(f);	

		for(int i=0; i<4; i++)
		{
			printf("\n Check 1");
			memset(config_file_cmp,0,30);
			sprintf(config_file_cmp,"%s%d","DFS",i+1);
			printf("\n The config_file_cmp value is: %s",config_file_cmp);
			
			ptr = strstr(config_file_buffer,config_file_cmp);
			if(ptr != NULL)
			{
				sscanf(ptr,"%s %s %s",dfc.name[i],dfc.server[i],dfc.port[i]);
				printf("\n The server val is: %s",dfc.server[i]);
				printf("\n The port val is: %s",dfc.port[i]);
			}
			else
			{
				printf("++++++ERROR:reading address and port number");
				return ERROR;
			}
		}
		
		//memset(config_file_cmp,0,30);
		ptr = strstr(config_file_buffer,"Username_password");
		ptr = ptr+18;
		if(ptr != NULL)
		{
			sscanf(ptr,"%s %s",dfc.username,dfc.pwd);
			strcpy(packet_info.username,dfc.username);
			printf("\n The username is %s",dfc.username);
			printf("\n The copied username is %s", packet_info.username);
			printf("\n The pwd is %s",dfc.pwd);
			return SUCCESS;
		}
		else
		{
			printf("++++++ERROR:reading username amd pwd");
			return ERROR;
		}
	}
	else
	{
		printf("++++++ERROR:In opening the config file");
		return ERROR;

	}
}


/* Calculation of md5sum*/
char *str2md5(const char *str, int length) 
{
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)malloc(33);

    MD5_Init(&c);

    while (length > 0) {
        if (length > 512) {
            MD5_Update(&c, str, 512);
        } else {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n) {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }

    return out;
}


/* Function for PUT command*/
status PUT()
{
	printf("\n ------- Entered the PUT function");
	FILE *fil;
	char last_char;
	long int file_size = 0;
	int server_index =0;
	int server_val = 0;
	int32_t ifile_size = 0;
	int file_read[4] = {0};
	char *file_buffer_send[4];
	char mk_dir[100];
	
	
	fil = fopen(packet_info.filename,"r");
	if(fil != NULL)
	{
		if(packet_info.subfolder != NULL)
		{
			sprintf(mk_dir,"mkdir -p %s/%s",packet_info.username,packet_info.subfolder);
			system(mk_dir);
		}

		fseek(fil,0,SEEK_END);
		file_size = ftell(fil);
		printf("\n file_size_check: %ld",file_size);
		
		
		fseek(fil,0,SEEK_SET);
		char *file_buffer = (char *)calloc(1,file_size);
		if(file_buffer == NULL)
			printf("\n +++++ERROR: Creating malloc");
		fread(file_buffer,1,file_size,fil);
		fseek(fil,0,SEEK_SET);

		// call md5sum function
		char *md5sum_value = str2md5(file_buffer, strlen(file_buffer));

		printf("\n The md5sum value is: %s\n", md5sum_value);
		int len = strlen(md5sum_value);

		last_char = md5sum_value[len-1];
		free(file_buffer);
		file_buffer = NULL;
		
        if (last_char=='a'){last_char='0';}
        else if (last_char=='b'){last_char='1';}
        else if (last_char=='c'){last_char='2';}
        else if (last_char=='d'){last_char='3';}
        else if (last_char=='e'){last_char='4';}
        else if (last_char=='f'){last_char='5';}

    	printf("\n The last char is %c", last_char);
		
		x = last_char % 4;
		printf("\n The x value is: %d", x);
		
		ifile_size = (int)(file_size/4);
		printf("\n The file size in int is:%d", ifile_size);

		for(int i=0 ; i<4 ; i++)
		{
        	file_buffer_send[i] = (char *)calloc(1,ifile_size);
        	if(file_buffer_send[i] == NULL)
        		printf("+++++++++++++ERROR: Creting malloc");
		}
		
		for(int index =0 ; index<4 ; index++)
		{
			if(index != 3)
			{
				file_read[index] = fread(file_buffer_send[index],1,ifile_size,fil);
				printf("\n The file size read for %d is:%d", index, file_read[index]);
			}
			else
			{
				ifile_size = (file_size- 3*ifile_size);	
				file_read[index] = fread(file_buffer_send[index],1,ifile_size,fil);
				printf("\n The file size read for %d is:%d", index , file_read[index]);
			}
		
		}

		for(packet_info.pieces=0;packet_info.pieces<4;packet_info.pieces++)
			{
				LUT(x);

				packet_info.file_piece_length = file_read[packet_info.pieces];
				printf("\n The file piece is%d size- %ld", packet_info.pieces, packet_info.file_piece_length);

				if(packet_info.file_piece_length%BUF_SIZE==0)
					packet_info.packetcount = (packet_info.file_piece_length/BUF_SIZE);
				else
					packet_info.packetcount = (packet_info.file_piece_length/BUF_SIZE)+1;
			
				
				FILE *f;
				for (server_index =0 ; server_index<2;server_index++)
				{

					printf("\n---------Entered the condition to send data");
					printf("\n The data is sent to :  %d",a[server_val]);
					printf("\n The authentication status is: %d", ipassword_match[a[server_val]]);
					if(ipassword_match[a[server_val]] == 1)
					{
						
						printf("\n Aunthentication Success");
						int send_status = send(cli_sock[a[server_val]],&packet_info,sizeof(packet_info),0);
						printf("\n The send status is: %d",send_status);
						printf("\n The command is: %s",packet_info.command_choice);

						
						f = fopen("buffer.txt","w+");

						int fwrite_status = fwrite(file_buffer_send[packet_info.pieces],1,packet_info.file_piece_length,f);
						printf("\n fwrite_status is %d", fwrite_status);
						fseek(f,0,SEEK_SET);
						memset(filebuffer,0,BUF_SIZE);

						
						for(long int i =0; i<packet_info.packetcount; i++)

						{
							int fread_status = fread(filebuffer,1,BUF_SIZE,f);
							printf("\n The fread status is %d",fread_status);
							for(long int index=0; index<fread_status; index++)
								{
									filebuffer[index] ^= key;
								}
							send(cli_sock[a[server_val]],&fread_status,sizeof(int),0);
							send(cli_sock[a[server_val]],filebuffer,fread_status,0);

						}
						
					
					}

					else
						printf("\n +++++ERROR: Sending data: No Authentication");
					
					server_val++;
				}

				
			} 
		for(int i=0 ; i<4 ; i++)
		{
			free(file_buffer_send[i]);
			file_buffer_send[i] = NULL;
		}
		
		fclose(fil);
		return SUCCESS;
	}
	else
	{
		printf("\n The requested file is not present ");
		return ERROR;
	}

}

/*Function for List*/
status list()
{
	char buffer [4][BUF_SIZE];
	char buffer_search[BUF_SIZE];
	char buf[60];
	char buf_chunk[60];
	char buf_list[500];
	printf("\n Entered the list command");
	printf("\n The file to be listed is %s", packet_info.filename);

	memset(buffer_search,0,BUF_SIZE);
	memset(buf_list,0,500);

	for(int i=0; i<4; i++)
		memset(buffer[i],0,BUF_SIZE);

	for(int i =0; i<4; i++)
	{
		if(ipassword_match[i] == 1)
		{
			send(cli_sock[i],&packet_info,sizeof(packet_info),0);
			recv(cli_sock[i],buffer[i],sizeof(buffer),0);
			strcat(buffer_search,buffer[i]);
		}
		else
			printf("\n +++++ERROR: No Authentication");
		
		
	}

	printf("\n The list is%s", buffer_search);
	FILE *f;
	FILE *f2;
	char *ptr0;
	char *ptr;
	char *ptr2;
	char *dup_check;
	char num [5];
	int count = 0;
	char buf_final[BUF_SIZE];
	char buf_incomplete[BUF_SIZE];
	//char ch = " ";
	f = fopen("search.txt","w");
	f2 = fopen("search.txt","r");

	memset(buf_final,0,BUF_SIZE);
	memset(buf_incomplete,0,BUF_SIZE);
	memset(buf,0,60);

	if(f != NULL && f2 != NULL)
	{
		int fwrite_status = fwrite(buffer_search,1,strlen(buffer_search),f);
		printf("The fwrite status is: %d", fwrite_status);
		fseek(f,0,SEEK_SET);

		while(fgets(buf,60,f2)!=NULL)
		{
			//printf("\n the content is %s",buf);
			ptr0 = strstr(buf," ");
			if( ptr0 != NULL)
			{

				ptr = strrchr(buf,' ');
				*ptr = '\0';
				if(ptr != NULL)
				{
					printf("the file name to search is: %s", buf);
					
					count =0;
					for(int i =0;i<4;i++)
					{
						memset(num,0,5);
						strcpy(buf_chunk,buf);
						sprintf(num," %d",i);
						strcat(buf_chunk,num);
						ptr2 = strstr(buffer_search,buf_chunk);
						
						if(ptr2 != NULL)
						{
							count++;
						}
						
			
					}
					printf("\n The count is:%d",count);
					if(count == 4)
						{
							dup_check = strstr(buf_list,buf);
							if(dup_check == NULL)
							{
								strcat(buf_list,buf);
								strcat(buf_list,"\n");
								
							}
							

						}
						else

						{
							dup_check = strstr(buf_list,buf);
							if(dup_check == NULL)
							{
								strcat(buf_list,buf);
								strcat(buf_list," {incomplete}");
								strcat(buf_list,"\n");
								
							}

						}

				}

			}

		}
			printf("\n The buffer list is: \n%s",buf_list);

			fclose(f);
			fclose(f2);
	}

	return SUCCESS;

}

/*Function for get*/
status get()
{
	printf("\n ---------Entered the get function");
	int flag = 0;
	int filesize =0;
	char fil_open[20];
	memset(fil_open,0,20);
	FILE *f;

	for(int i =0; i<4; i++)
	{
		if(ipassword_match[i] == 1)
		{
			send(cli_sock[i],&packet_info,sizeof(packet_info),0);
			printf("\n cehck 1");
			
			for(int serv_recv = 0; serv_recv<2; serv_recv++)
			{
				printf("\n The data received from Server %d",i);
				memset(&get_info,0,sizeof(get_info));
				recv(cli_sock[i],&get_info,sizeof(get_info),0);
				printf("\n The packet count value is %d",get_info.packetcount);
				printf("\n The file to be created is %s",get_info.filename);
				if(get_info.packetcount != 0)
				{
					f = fopen(get_info.filename,"w");
					if(f!=NULL)
					{
						printf("check 2");
						memset(filebuffer,0,BUF_SIZE);
						for(long int index =0; index<get_info.packetcount; index++)
						{
							recv(cli_sock[i],&filesize,sizeof(int),0);
							recv(cli_sock[i],filebuffer,filesize,0);
							for(long int index=0; index<filesize; index++)
							{
								filebuffer[index] ^= key;
							}
							printf("The file content received is %s",filebuffer);
							int fwrite_status = fwrite(filebuffer,1,filesize,f);
							printf("\n the write status is: %d",fwrite_status);
						}
						fclose(f);

					}
					else
						printf("\n The file cannot be opened");


				}
				else
				{
					flag = 1;
					
				}
				
			


			}

		}
		else
			printf("\n +++++ERROR: No Authentication");
		
		
	}
	/*checking if all files exist*/
	if(flag ==1)
		printf("\n-------- The requested file/subfolder is not available on the server");
	else
	{

		FILE *fil1;
		int count =0;

		for(int i =0;i<4; i++)
		{
			char path_check[100];
			if(packet_info.subfolder!= NULL)
			{
				sprintf(path_check,"%s/%s/%s %d",packet_info.username,packet_info.subfolder,packet_info.filename,i);
			}
			else
				sprintf(path_check,"%s/%s %d",packet_info.username,packet_info.filename,i);
			printf("\n The path_check file is:%s",path_check);


			fil1 =fopen(path_check,"r");
			if(fil1 != NULL)
			{
				fclose(fil1);
				count++;
			}
		}

		if(count == 4)
		{
			printf("All the chunks are present");

			char path[100];
			char path_read[100];
			FILE *fil;
			FILE *file_read;
			int file_size =0;

			if(packet_info.subfolder != NULL)
					{
						strcpy(path,packet_info.username);
						strcat(path,"/");
						strcat(path,packet_info.subfolder);
						strcat(path,"/");
						strcat(path,packet_info.filename);

					}
					else
					{
						strcpy(path,packet_info.username);
						strcat(path,"/");
						strcat(path,packet_info.filename);

					}
			fil=fopen(path,"w");
			if(fil != NULL)
			{
				for(int i=0;i<4;i++)
				{
					sprintf(path_read,"%s %d",path,i);		
					file_read=fopen(path_read,"r");
					if(file_read != NULL)
					{
						struct stat st;
						stat(path_read,&st);
						file_size = st.st_size;

						char *buf = (char*)malloc(file_size);
						if(buf == NULL)
							printf("\n ++++ERROR: creating malloc");
						int fread_status = fread(buf,1,file_size,file_read);
						printf("\n the file read status is: %d",fread_status);
						int fwrite_status = fwrite(buf,1,file_size,fil);
						printf("\n the file write status is: %d",fwrite_status);
						fclose(file_read);
						remove(path_read);
					}
					else
						printf("\n ++++++++Problem in opening the chunk files");

				}
				fclose(fil);
			}
			else
				printf("\n ++++++Error: Opening the main file to write");
		}
		else
			printf("\n chunks are missing");
	}
	

	return SUCCESS;

}

status mkdirectory()
{
	for(int i =0; i<4; i++)
	{
		send(cli_sock[i],&packet_info,sizeof(packet_info),0);	
	}
	

return SUCCESS;
}


int main(int argc, char *argv[])
{
	status return_status;
	int set = 0;
	int ret_status = 0;
	struct timeval set_time={1,0};

	if (argc < 2)
        {
                printf("\n The arguments is missing");
                exit(1);
        }
	else
	{
		printf("\n The arguments are entered correctly");
		return_status = config_file(argv[1]);

		if (return_status == ERROR)
			printf("\n ++++++ERROR- Opening the config file");
	}


	struct sockaddr_in dfs_address[4];
	char user_validation[100];
	
	for (int i =0; i<4; i++)
	{
		dfs_address[i].sin_family = AF_INET;
		dfs_address[i].sin_addr.s_addr = inet_addr(dfc.server[i]); //host to network long
        	dfs_address[i].sin_port = htons(atoi(dfc.port[i])); //host to network short
	}


	for(int i=0; i<4; i++)
		{
			cli_sock[i]= socket(AF_INET,SOCK_STREAM,0);
			if(cli_sock[i] < 0)
				printf("\n ++++++++ ERROR: Creating socket");
			
			ret_status = connect(cli_sock[i], (struct sockaddr *) &dfs_address[i], sizeof(dfs_address[i]));
			if(ret_status < 0)
				printf("\n ++++++++ ERROR: Connection socket");	
			else
			{	
				printf("\n Connection Successful");
			}
			setsockopt(cli_sock[i], SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set));
			setsockopt(cli_sock[i], SOL_SOCKET, SO_RCVTIMEO,(char*)&set_time,sizeof(struct timeval));
		}

	signal(SIGPIPE, SIG_IGN);
		
	while(1)
	{
		printf("\n\n Enter the desired operation to perform");
		printf ("\n 1 get [filename]");
        printf ("\n 2 put [filename]");
		printf ("\n 3 list");
		printf ("\n 4 mkdir");

		
		
		memset(command,0,30);
        
        //memset(packet_info.filename,0,40);
		

		fgets(command,30,stdin);
		printf("\n Entered input is: %s \n", command);
		
		if(return_status != ERROR)
		{
			for(int i=0; i<4; i++)
				ipassword_match[i] =0;

			memset(&packet_info,0,sizeof(packet_info));

			for(int i=0; i<4; i++)
			{
				sprintf(user_validation,"%s %s",dfc.username,dfc.pwd);
				strcpy(packet_info.username,dfc.username);
            	printf("\n The user validation is: %s", user_validation);
            	send(cli_sock[i],user_validation,100,0);//1
                recv(cli_sock[i],&ipassword_match[i],sizeof(int),0);//-1
			
				if(ipassword_match[i])
					printf("\n The password is match for %d",i+1);
				else
					printf("\n +++++ The password is not a match for %d",i+1);
			
			}

		}
		
		sscanf(command,"%s %s %s",packet_info.command_choice,packet_info.filename,packet_info.subfolder);
		printf("\n The command is: %s",packet_info.command_choice);
		printf("\n The file name is: %s",packet_info.filename);
		printf("\n The subfolder is: %s", packet_info.subfolder);

		

		// PUT Command 
		if(strcmp(packet_info.command_choice,"put")==0 && (packet_info.filename != NULL))
		{
			return_status = PUT();	
			if(return_status == ERROR)
				printf("\n The requested file is not present");
		}

		else if(strcmp(packet_info.command_choice,"list")==0)
		{
			if(packet_info.filename != NULL)
				strcpy(packet_info.subfolder,packet_info.filename);
			printf("\n The subfolder is: %s", packet_info.subfolder);
			return_status = list();
		}
		else if(strcmp(packet_info.command_choice,"get")==0)
		{
			return_status = get();
		}
		else if(strcmp(packet_info.command_choice,"mkdir")==0)
		{
			if(packet_info.filename != NULL)
				strcpy(packet_info.subfolder,packet_info.filename);
			return_status = mkdirectory();
		}
		else
			printf("\n Invalid Command. Please re-enter");


	
	} //end while

return 0;
} // end main
