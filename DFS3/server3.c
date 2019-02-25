#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<dirent.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<dirent.h>
#include<sys/stat.h>

#define BUF_SIZE 2048

int client_socket =0;
int flag =0;

char command[30];
char command_choice[8];
char file_name[20];
char buffer[BUF_SIZE];
char key;


typedef enum{
        ERROR = -1,
        SUCCESS = 1,
}status;

status return_status;

typedef struct{
        long int seq_no;
        char buffer[BUF_SIZE];
        long int frame_size;
}frame;

typedef struct{
	int packetcount;
	char filename[50];
}get_t;

get_t get_info;



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

status user_validation()
{
	char user_validation[100];
        char fil_buf[500]; 	
	FILE *f;
	char *ptr;
	int fil_size_check = 0;
	int ipassword_match = 0;
	recv(client_socket,user_validation,100,0);//1
	printf("\n The received username_pwd is: %s",user_validation);
	f = fopen("dfs.config","r");

	if(f != NULL)
	{
		memset(fil_buf,0,500);

                fseek(f,0,SEEK_END);
                fil_size_check = ftell(f);
                printf("\n file_size_check: %d",fil_size_check);
                fseek(f,0,SEEK_SET);
                fread(fil_buf,1,fil_size_check,f);
		ptr = strstr(fil_buf,user_validation);
		if(ptr != NULL)
		{
			
			printf("\n Username and pws match");
			ipassword_match = 1;
			send(client_socket,&ipassword_match,sizeof(int),0);//-1
			return SUCCESS;
		}
		else 
		{
			printf("\n +++++++User name and pwd do not match");
			ipassword_match = 0;
            send(client_socket,&ipassword_match,sizeof(int),0);//-1
			return ERROR;
		}
		fclose(f);
	}
	else
	{
		printf("\n ++++++++ERROR: Opening config file");
		return ERROR;
	}
}

/*PUT Funciton*/
status PUT()
{
	
	char mk_dir[50];
	printf("\n------Entered the PUT function");
	printf("\n The size of the file to be created is: %ld", packet_info.file_piece_length);
	printf("\n The piece received at server1 is :%d", packet_info.pieces);
	printf("\n The filename to be created is: %s", packet_info.filename);
	printf("\n The number of packets is: %d",packet_info.packetcount);

	if(packet_info.packetcount > 0)
	{
		FILE *f;
		char file_name[50];
		char username[50];
		strcpy(username,packet_info.username);
		printf("\n The username is:%s", username);
		
		char file_buffer_receive[BUF_SIZE];
		int packet_size =0;

		if(packet_info.subfolder != NULL)
		{
			sprintf(mk_dir,"mkdir -p %s/%s",username,packet_info.subfolder);
			system(mk_dir);
			sprintf(file_name,"%s/%s/%s %d",username,packet_info.subfolder,packet_info.filename,packet_info.pieces);
		}
		else
		{
			sprintf(mk_dir,"mkdir -p %s",username);
			system(mk_dir);
			sprintf(file_name,"%s/%s %d",username,packet_info.filename,packet_info.pieces);

		}

		printf("\n The file name that needs to be created is: %s",file_name);
		
		memset(file_buffer_receive,0,BUF_SIZE);
		
		f = fopen(file_name,"w");

		if(f!=NULL)
		{
			printf("\n -----Entered the function to write file");
			for(int i=0; i<packet_info.packetcount;i++)
			{
				recv(client_socket,&packet_size,sizeof(int),0);
				printf("\nThe packet_size received is: %d",packet_size);
				recv(client_socket,file_buffer_receive,packet_size,0);
				for(long int index=0; index<packet_size; index++)
				{
					file_buffer_receive[index] ^= key;
				}
				int fwrite_status = fwrite(file_buffer_receive,1,packet_size,f);
				
				printf("\n The status of write is: %d", fwrite_status);

			}
			
			fclose(f);

		}
		else
		{
			printf("\n ++++++++ ERROR: The file cannot be opened to write data");
			return ERROR;

		}


	}

	return SUCCESS;
}

void listdir(const char *name, int indent)
{
    DIR *dir;
    struct dirent *entry;
    char buffer_coy[20];
    //memset(buffer,0,BUF_SIZE);
    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            //printf("%*s[%s]\n", indent, "", entry->d_name);
            listdir(path, indent + 2);
        } else {
            //printf("\n LIST:%*s- %s\n", indent, "", entry->d_name);
            strcpy(buffer_coy,entry->d_name);
            printf("\n LIST---- %s", entry->d_name);
            strcat(buffer,buffer_coy);
            strcat(buffer,"\n");
        }
    }
    closedir(dir);
}

/*LIST function*/
status list()
{
	char path[100];

	memset(buffer,0,BUF_SIZE);
	printf("\n ----- Entered the function to list");
	//recv(client_socket,&packet_info,sizeof(packet_info),0);
	printf("\n The user name is:%s", packet_info.username);
	strcpy(path,"/");
	if(packet_info.subfolder != NULL)
	{
		printf("\n check 1");
		sprintf(path,"%s/%s",packet_info.username,packet_info.subfolder);
	}
	else
		sprintf(path,"%s",packet_info.username);

	listdir(path, 0);
	 printf("\n LIST buffer:%s", buffer);
	if(flag ==1){
		send(client_socket,buffer,strlen(buffer),0);
	}
		return SUCCESS;

}



/*GET function*/
status get()
{
	char buf[60];
	char path[100];
	char dir_check[100];
	char *ptr;
	FILE *f1;
	FILE *f2;
	FILE *f3;
	int file_size =0;
	char filebuffer[BUF_SIZE];

	memset(buffer,0,BUF_SIZE);
	memset(dir_check,0,100);
	memset(path,0,100);

	printf("\n Entered the GET function");
	printf("\n The file to be fetched is: %s",packet_info.filename);
	flag =0;
	return_status = list();

	if(packet_info.subfolder != NULL)
	{
		strcpy(dir_check,packet_info.username);
		strcat(dir_check,"/");
		strcat(dir_check,packet_info.subfolder);
	}

	DIR* dir = opendir(dir_check);
	if (dir)
	{
	    printf("\nDirectory exists.");
	    /* Directory exists. */
	    closedir(dir);
	    printf("\n The buffer value is: %s", buffer);
		f1 = fopen("search.txt","w");
		f2 = fopen("search.txt","r");

		memset(&get_info,0,sizeof(get_info));


		if(f1 != NULL && f2 != NULL)
		{
			int fwrite_status = fwrite(buffer,1,strlen(buffer),f1);
			printf("The fwrite status is: %d", fwrite_status);
			fseek(f1,0,SEEK_SET);

			while(fgets(buf,60,f2)!=NULL)
			{
				ptr = strstr(buf,packet_info.filename);
				if(ptr != NULL)
				{
					printf("\n File is present");
					printf("\n The file name is: %s",buf);

					printf("\n The user name is:%s", packet_info.username);
					printf("\n the strlen of username is:%ld",strlen(packet_info.username));
					printf("\n the strlen of buf is:%ld",strlen(buf));
					if(packet_info.subfolder != NULL)
					{
						strcpy(path,packet_info.username);
						strcat(path,"/");
						strcat(path,packet_info.subfolder);
						strcat(path,"/");
						strcat(path,buf);

					}
					else
					{
						strcpy(path,packet_info.username);
						strcat(path,"/");
						strcat(path,buf);

					}
					printf("\n The path to open is:%s",path);
					//printf("\n the strlen of path is %ld",strlen(path));
					path[strlen(path)-1] = '\0';
					printf("\n the strlen of path is %ld",strlen(path));
					//printf("\n the strlen of path is %ld",strlen("srinath/test1.txt 0"));

					f3 = fopen(path,"r");
					if(f3!=NULL)
					{
						printf("\n File opened");
						struct stat st;
						stat(path,&st);
						file_size = st.st_size;

						printf("\nThe size of the file is:%d",file_size);
						strcpy(get_info.filename,path);

						memset(filebuffer,0,BUF_SIZE);
						//printf("\nThe size of the file is: %d",file_size);
						if(file_size%BUF_SIZE==0)
							get_info.packetcount = (file_size/BUF_SIZE);
						else
							get_info.packetcount = (file_size/BUF_SIZE)+1;

						printf("\n The packet caount value is %d",get_info.packetcount);

						int send_status1 = send(client_socket,&get_info,sizeof(get_info),0);
						printf("\n The send status1: %d",send_status1);
						for(long int i =0; i<get_info.packetcount; i++)

						{
							int filesize = fread(filebuffer,1,BUF_SIZE,f3);
							printf("\n The fread status is %d",filesize);
			 
							int send_status2 = send(client_socket,&filesize,sizeof(int),0);
							printf("\n The send status2: %d",send_status2);
							for(long int index=0; index<filesize; index++)
							{
								filebuffer[index] ^= key;
							}
							int send_status3 = send(client_socket,filebuffer,filesize,0);
							printf("\n The send status3: %d",send_status3);
						}
						fclose(f3);	

					}
					else
					{
						printf("\n ++++ERROR: FIle not opened");
					}
				}
				else
				{
					printf("\n File is not present");
			}

			}
		fclose(f1);
		fclose(f2);
		}	
	}
	else
	{
		printf("\n The subfolder is not present");
		get_info.packetcount=0;
		send(client_socket,&get_info,sizeof(get_info),0);
		send(client_socket,&get_info,sizeof(get_info),0);
	}


	return SUCCESS;


}

status mkdirectory()
{
	char mk_dir[100];
	sprintf(mk_dir,"mkdir -p %s/%s",packet_info.username,packet_info.subfolder);
	system(mk_dir);
	return SUCCESS;
}

int main(int argc, char *argv[]) {
	
	
	int length = 0;
	int server_socket =0;
	//int n = 0;
	int set = 0;
	int portno = 0;
	int listen_status = 0;
	struct sockaddr_in server_address,client_address;
	memset(&packet_info,0,sizeof(packet_info));
	//char user_validation[100];

	portno = atoi(argv[2]);
	length = sizeof(client_address);
	if (argc != 3)
        {
                printf("\n The arguments is missing");
                exit(1);
        }
        else
        {
                printf("\n The arguments are entered correctly");
	}
	server_socket = socket(AF_INET,SOCK_STREAM,0);
        if (server_socket<0)
                printf("\n ERROR:Opening socket");

	server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
    server_address.sin_port = htons((unsigned short)portno); //host to network short

        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set));
	// binding the socket
        if(bind(server_socket, (struct sockaddr *) &server_address , sizeof(server_address))<0)
                printf("\n ERROR:Binding Failed");
        else
                printf("\n Binding done");
        // listening for requests from client
        listen_status = listen(server_socket,8);

	if(listen_status <0)
		printf("\n Not listening");
	else
		printf("\n Listening");

	while(1)
	{
		client_socket = accept(server_socket, (struct sockaddr *) &client_address,(socklen_t*) &length);
		while(client_socket > 0)
		{

			printf("\n Client connection success with Server 1");
			//while(1)
			//{
			return_status = user_validation();
			
			if(return_status == SUCCESS)
			{
				printf("\n User is legitimate user");
				
				for(int serv =0; serv<2; serv++)
				{
					int recv_status = recv(client_socket,&packet_info,sizeof(packet_info),0);
					printf("\n The receive status is %d", recv_status);
				
					printf("\n The packet command received is: %s\n", packet_info.command_choice);

					if(strcmp(packet_info.command_choice,"put")==0 && (packet_info.filename != NULL))
					{
						return_status = PUT();
						//printf("\n the execution time is: %d", serv);
					}
					else if(strcmp(packet_info.command_choice,"list")==0)
					{
						flag =1;
						return_status = list();
						serv = 2;
					}
					else if(strcmp(packet_info.command_choice,"get")==0)
					{
						return_status = get();
						serv = 2;	
					}
					else if(strcmp(packet_info.command_choice,"mkdir")==0)
					{
						return_status = mkdirectory();
						serv = 2;	
					}
				}
			}	
			else
			{
				printf("\n pwd do not match. try again");

			}
			


	}
	}

return 0;
}
