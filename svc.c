#include "svc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

struct commit
{

 char* message;
 int merged;
 char* branch_under;
 char* commitId;
 int filesTracked;
 int removed;
 struct commit *parent;
 struct files *tracked;

};

struct branches
{
  int merged; // merged called --> 1 when called and then make 0 again
 char *name;
 int commitCount;
 struct commit *listOfCommits;
 struct commit *HeadCommit;
};

struct svc
{
 int countBranches;
 int countFile;
 int deletedFileCount;
 int TotalCommit;
 struct branches* active;
 struct files **ListOfFiles;

 struct branches **ListBranches;
 struct commit **ListOfCommits;

};
struct files
{
  int resolution;
 int statusTracked;
 char* name;
 int hash_value;
 int removed; // 0 is false and 1 is true;
 int new;
 int new_merged;
};

void *svc_init(void)
{
//setting up SVC and master branch
 struct svc *s = malloc(sizeof(struct svc));
 struct branches *b = malloc(sizeof(struct branches));
 b->HeadCommit = NULL;
 b->commitCount = 0;
 // set currently alive branch to "master"
 s->active = b;
 s->countBranches = 0;
 s->ListBranches = malloc(sizeof(struct branches*)*10);
 s->ListBranches[s->countBranches] = b;
 s->ListBranches[0]->name = malloc(sizeof(char)*10);
 s->TotalCommit = 0;
 strcpy(s->ListBranches[s->countBranches]->name,"master");
 s->countBranches +=1;
 s->countFile= 0;
 s->deletedFileCount = 0;
//returning pointer of SVC memory
 return s;
}

void addFile(void *helper, char *file_path, int hash_value)
{
 struct svc *data = (struct svc *)helper;
// if there is no files in svc make memory for listof files in SVC
 if(data->countFile == 0)
 {
   //making a new file
   struct files *f = malloc(sizeof(struct files));
   f->removed = 0;
   f->hash_value = hash_value;
   f->statusTracked = 0;
   f->new = 0;
   f->resolution = 0;
   f->new_merged = 0;
   data->ListOfFiles = malloc(sizeof(struct files*)*10);
   data->ListOfFiles[data->countFile] = f;
   data->ListOfFiles[data->countFile]->name = malloc(sizeof(char)*50);
   strcpy(data->ListOfFiles[data->countFile]->name, file_path);
   data->countFile +=1;
 }
// if there is already files, realloc the memoery
 else if(data->countFile>0)
 {
   struct files *f = malloc(sizeof(struct files));
   f->removed = 0;
   f->hash_value = hash_value;
   f->new = 0;
   data->ListOfFiles = realloc(data->ListOfFiles,sizeof(struct files*)*10);
   data->ListOfFiles[data->countFile] = f;
   data->ListOfFiles[data->countFile]->name = malloc(sizeof(char)*50);
   strcpy(data->ListOfFiles[data->countFile]->name, file_path);
   data->countFile +=1;
 }
}

void cleanup(void *helper)
{

 struct svc *data = (struct svc *)helper;
//remove all files memory
 for (int i = 0 ; i < data->countFile; i++)
 {
   if(data->ListOfFiles[i] != NULL)
   {
     free(data->ListOfFiles[i]->name);
     free(data->ListOfFiles[i]);
   }
 }

 for (int i = 0 ; i < data->TotalCommit; i++)
 //remove  all memory associated with commits
 {
   if(data->ListOfCommits[i] != NULL)
   {
     for(int j =0; j < data->ListOfCommits[i]->filesTracked; j++)
     {
       free(data->ListOfCommits[i]->tracked[j].name);
     }
     if(data->ListOfCommits[i]->branch_under != NULL)
     {
       free(data->ListOfCommits[i]->branch_under);
     }

     free(data->ListOfCommits[i]->parent);
     free(data->ListOfCommits[i]->commitId);
     free(data->ListOfCommits[i]->message);
     free(data->ListOfCommits[i]->tracked);
     free(data->ListOfCommits[i]);
   }
 }
 if(data->TotalCommit != 0)
 {
   //no commits made just free commit list
   free(data->ListOfCommits);
 }


 if(data->countFile != 0)
 {
  // if no files added just free file list
   free(data->ListOfFiles);
 }

// remove associated memory with branch
 for(int j =0 ; j < data->countBranches; j++)
 {
   free(data->ListBranches[j]->name);
   if(data->ListBranches[j]->commitCount != 0)
   {
     free(data->ListBranches[j]->listOfCommits);
   }
   free(data->ListBranches[j]);
 }

 if(data->countBranches != 0)
 {
   free(data->ListBranches);
 }
// free SVC
 free(data);
}

void lower_string(char s[])
{
  //function that converts a given char to lowercase
  int c = 0;

  while (s[c] != '\0') {
     if (s[c] >= 'A' && s[c] <= 'Z') {
        s[c] = s[c] + 32;
     }
     c++;
  }
}

int hash_file(void *helper, char *file_path)
{

   int asciiFile = 0;
   int hash  = 0;
   int c = 0;
   FILE *fp;
   int asciiContext = 0;

   // Can't find file
   fp = fopen(file_path, "r");
   if (file_path == NULL)
   {
     return -1;
   }

   //can't open file or not in directory
   if (fp == NULL)
   {
     return -2;
   }
   //ascii calculation of file_path
   for(int i = 0; i < strlen(file_path);i++)
   {
     asciiFile += file_path[i];
   }
   hash = (hash + asciiFile) %1000;

   // asscii calculations for file_context
   while((c=fgetc(fp)) !=EOF)
   {
     asciiContext += c;
   }

   hash=(hash+asciiContext) % 2000000000;

   fclose(fp);
   return hash;

}



char *svc_commit(void *helper, char *message) {
   int id = 0;
   int c = 0;
   int d = 0;

   struct svc *data = (struct svc *)helper;
   // no message
   if(message == NULL)
   {
     return NULL;
   }

   // files in svc is none
   if(data->countFile == 0)
   {
     return NULL;
   }

   //no Changes
   int stat = 0;
   for(int i = 0; i < data->countFile; i++)
   {
     if(data->ListOfFiles[i]->removed != 1)
     {
       if(data->ListOfFiles[i]->statusTracked ==1 && (hash_file(helper,data->ListOfFiles[i]->name) == data->ListOfFiles[i]->hash_value))
       {
         stat +=1;
       }
     }
   }
   if(stat == data->countFile)
   {
     return NULL;
   }

   //perform calculation for the btyes in message
   for(int i = 0; i <strlen(message); i++)
   {
     c = message[i];
     id = (id + c) %1000;
   }

   //arrange the files in alphabetical order
   char **array = malloc(50 * sizeof(char *));
   int i;
   int count = 0;
   for (i = 0; i < data->countFile; ++i) {
       array[i] = (char *)malloc(50);
       strcpy(array[i],data->ListOfFiles[i]->name);
       count +=1;
   }
   for (int i = 0; i < count; i++)
   {
      for (int j = i+1; j < count; j++)
      {
          char d[100];
          char e[100];

          strcpy(d,array[i]);
          strcpy(e,array[j]);
          lower_string(d);
          lower_string(e);

          if (strcmp(d,e) > 0)
          {
            char* temp = array[i];
            array[i] = array[j];
            array[j] = temp;
          }
      }
   }
// First commit
   if(data->active->HeadCommit == NULL)
   {
     int status = 0;
     int add = count;

     for(int i = 0; i < count; i++)
     {
       //if file manually removed
       if(hash_file(helper,array[i]) == -2){
         status = 1;
       }

       if(add>0 && status != 1)
       {
         // if is addition
         add-=1;
         id = id + 376591;
       }
       if(status ==0)
       {
         //ascii calculations performed on file name
         for(int j = 0; j < strlen(array[i]); j++)
         {
           d = array[i][j];
           id = (id * (d%37)) % 15485863 + 1;
         }
       }
        status = 0;
     }
   }

   else if (data->active->HeadCommit != NULL)
   {
      int status = 0;
      char *name_found = NULL;
      int position = 0;
      int subtracted = 0;


      for(int i = 0; i < count; i++)
      {
        // manually removed
        if(hash_file(helper,array[i]) == -2)
        {
          id = id + 85973;
          status = 1;
          subtracted = 1;
        }

        for(int k= 0; k< data->active->HeadCommit->filesTracked; k++)
        {
          if(strcmp(array[i],data->active->HeadCommit->tracked[k].name) == 0)
          {
            // found file
            name_found = array[i];
            position = k;
          }
        }
        //if false: file not found in svc
        if (name_found != NULL && subtracted == 0)
        {

          if(hash_file(helper,name_found) != data->active->HeadCommit->tracked[position].hash_value)
          {
            // change context
            id = id + 9573681;
            status = 1;
          }
          if(data->active->HeadCommit->tracked[position].removed == 1 )
          {
            //deletion
            id = id + 85973;
            status = 1;
            data->active->HeadCommit->tracked[position].removed = 2;
          }
          name_found = NULL;
        }
        else if (subtracted == 0)
        {
          //addition
          if(data->active->HeadCommit->filesTracked < count )
          {
            id = id + 376591;
            status =1;
          }
        }

        if(status == 1)
        {
          for(int j = 0; j < strlen(array[i]); j++)
          {
            //calculation on chnaged file_name
            id = (id * (array[i][j] %37)) % 15485863 + 1;
            status = 0;
            subtracted = 0;
          }
        }
      }
    }

//adding commits into svc
   if(data->active->commitCount == 0)
   {
     // make commit obkeject
     struct commit *c = malloc(sizeof(struct commit));
     data->active->listOfCommits = malloc(sizeof(struct commit));
     c->filesTracked = 0;
     data->TotalCommit = 0;
     c->parent = NULL;
     c->removed = 0;

     // an array of tracked creted
     c->tracked = malloc(sizeof(struct files)*20);
     for(int i = 0; i < data->countFile; i++)
     {
       // add to newest commit
       struct files *f = data->ListOfFiles[i];
       c->tracked[c->filesTracked] = *f;
       //rehash if this is changed
       int new = hash_file(helper, f->name);
       c->tracked[c->filesTracked].hash_value = new;
       c->tracked[c->filesTracked].name = malloc(sizeof(char)*20);
       strcpy(c->tracked[c->filesTracked].name,f->name);
       c->tracked[c->filesTracked].removed = c->removed;
       data->ListOfFiles[i]->statusTracked =1;
       c->filesTracked +=1;
     }

     // make the Commit Id
     c->commitId =malloc(sizeof(char)*20);
     sprintf(c->commitId,"%06x",id);
     c->message =malloc(sizeof(char)*20);
     strcpy(c->message, message);
     c->branch_under=malloc(sizeof(char)*20);
     strcpy(c->branch_under,data->active->name);
     // add commit into branch
     data->active->listOfCommits[data->active->commitCount] = *c;
     data->active->commitCount +=1;
     // assign head (current commit)
     data->active->HeadCommit = c;
     //add to SVC big commit list
     data->ListOfCommits = malloc(sizeof(struct commit*)*5);
     data->ListOfCommits[data->TotalCommit] = c;
     data->TotalCommit +=1;

     //freed
     for(int i = 0; i < data->countFile; i++)
     {
       free(array[i]);
     }

     free(array);
     return c->commitId;
   }
   else
   {
     //make new commits
     struct commit *c = malloc(sizeof(struct commit));
     //get countFile
     c->filesTracked = 0;
     c->removed = 0;
     int position = data->active->commitCount;

     // assign commit-> parent to the one before index before
     c->parent = malloc(sizeof(struct commit));
     (*c->parent) = data->active->listOfCommits[position-1];
     // tracked files update
     c->tracked = malloc(sizeof(struct files)*20);
     //make space for tracked and add in all files in svc
     for(int i = 0; i < data->countFile; i++)
     {
       // add to newest commit
       struct files *f = data->ListOfFiles[i];
       //add new hash if changed
       int new = hash_file(helper, f->name);
       c->tracked[c->filesTracked].hash_value = new;
       c->tracked[c->filesTracked].name = malloc(sizeof(char)*50);
       c->tracked[c->filesTracked].removed = c->removed;
       strcpy(c->tracked[c->filesTracked].name,f->name);
       // change status to tracked
       data->ListOfFiles[i]->statusTracked =1;
       c->filesTracked +=1;
       c->tracked[c->filesTracked] = *f;
     }
     //make space fo ID spcy ID
     c->commitId = malloc(sizeof(char)*40);
     sprintf(c->commitId,"%06x",id);
     c->message =malloc(sizeof(char)*50);
     strcpy(c->message, message);
     c->branch_under=malloc(sizeof(char)*20);
     strcpy(c->branch_under,data->active->name);
     //add to branch commit
     data->active->listOfCommits = realloc(data->active->listOfCommits,sizeof(struct commit)*10);
     data->active->listOfCommits[data->active->commitCount] = *c;
     data->active->commitCount +=1;
     //freed
     for(int i = 0; i < data->countFile; i++)
     {
       free(array[i]);

     }
     free(array);
     //add to SVC commit list
     data->ListOfCommits = realloc(data->ListOfCommits,sizeof(struct commit*)*10);
     data->ListOfCommits[data->TotalCommit] = c;
     data->TotalCommit +=1;

     //change current head commit
     data->active->HeadCommit = c;
     return c->commitId;
   }
   //freed
   for(int i = 0; i < data->countFile; i++)
   {
     free(array[i]);
   }
   free(array);

   return NULL;
}

void *get_commit(void *helper, char *commit_id)
{
  //search through active branch and find commit then return
   struct svc *data = (struct svc *)helper;
   struct commit *ptr;
   for(int i = 0; i < data->active->commitCount; i++)
   {
     if(strcmp(data->active->listOfCommits[i].commitId,commit_id) == 0 )
     {
       ptr = &data->active->listOfCommits[i];
       return ptr;
     }
   }
   return NULL;
}

char **get_prev_commits(void *helper, void *commit, int *n_prev)
{
   (*n_prev) = 0;
   int position = 0;
   int count = 0;

   struct svc *data = (struct svc *)helper;
   if(n_prev == NULL)
   {
     return NULL;
   }
   if(commit == NULL || data->active->commitCount == 1)
   {
     (*n_prev) = 0;
     return NULL;

   }

   for(int i = 0; i < data->active->commitCount; i++)
   {

     if(data->active->listOfCommits[i].removed == 0)
     {

       count +=1;
       if(commit == &data->active->listOfCommits[i])
       {
         position = i;
         break;
       }
     }
   }

   (*n_prev) = count-1;
   if(position == 0){
     return NULL;
   }

   char *c = NULL;
   char **parent = (char**) malloc(50 * sizeof(char*));
   for(int i = 0; i < position; i++)
   {
     if(data->active->listOfCommits[i].removed ==0)
     {
       c = data->active->listOfCommits[i].commitId;
       parent[i] = c;
     }

   }
   return parent;
}

int found(void *helper,int index, char*name)
{
  //function that checks if given file is in commit
  struct svc *data = (struct svc *)helper;
  // printf("this is index: %d\n",index);
  if(index != 0)
  {
    for(int i = 0; i < data->active->listOfCommits[index-1].filesTracked; i++ )
    {

      if(strcmp(name, data->active->listOfCommits[index-1].tracked[i].name)==0)
      {
        return 1;
      }
    }
  }
  return 0;
}

void print_commit(void *helper, char *commit_id)
{
  struct svc *data = (struct svc *)helper;
   // TODO: Implement
   int status = 0;
   int count = 0;
   if(commit_id == NULL)
   {
     printf("Invalid commit id\n");
     return;
   }

   for(int i = 0; i < data->active->commitCount; i++)
   {

     if(strcmp(data->active->listOfCommits[i].commitId,commit_id)==0)
     {
       status = 1;
       int printed = 0;
       // the branch its under
       printf("%s [%s]: %s\n",commit_id,data->active->listOfCommits[i].branch_under,data->active->listOfCommits[i].message);

       for(int j = (data->active->listOfCommits[i].filesTracked-1); j>=0; j--)
       {
          // printf("files tracked: %s\n",data->active->listOfCommits[i].tracked[0].name);

         if(hash_file(helper,data->active->listOfCommits[i].tracked[j].name) == -2)
         {

           //files manually deleted
           data->active->listOfCommits[i].tracked[j].removed =1;
         }
         if(data->active->listOfCommits[i].tracked[j].new_merged ==1 && data->active->listOfCommits[i].merged ==1)
         {
           printed =1;
           printf("    + %s\n",data->active->listOfCommits[i].tracked[j].name);
           data->active->listOfCommits[i].tracked[j].new_merged =0;
           data->active->listOfCommits[i].merged = 0;

         }
         // if not in previous one it would be a addition
         if(found(helper,i,data->active->listOfCommits[i].tracked[j].name) == 0 && printed != 1)
         {
           //if it is a addition
           printf("    + %s\n",data->active->listOfCommits[i].tracked[j].name);
           count +=1;

         }
         else{
           if(data->active->listOfCommits[i].tracked[j].removed ==1 && printed != 1)
           {
             //if files removed
             printf("    - %s\n",data->active->listOfCommits[i].tracked[j].name);
             count +=1;

           }
         }


       }
       printed =0;
       printf("\n");

       printf("    Tracked files (%d):\n",count);

       for(int k = 0; k <data->active->listOfCommits[i].filesTracked; k++)
       {
         if(found(helper,i,data->active->listOfCommits[i].tracked[k].name) ==0)
         {
           printf("    [%10d] %s\n",data->active->listOfCommits[i].tracked[k].hash_value,data->active->listOfCommits[i].tracked[k].name);
         }

         else
         {
           //if files not removed
           if(data->active->listOfCommits[i].tracked[k].removed != 1)
           {
             printf("    [%10d] %s\n",data->active->listOfCommits[i].tracked[k].hash_value,data->active->listOfCommits[i].tracked[k].name);
           }
         }
       }
     }
   }

   if(status == 0)
   {
     printf("Invalid commit id\n");
     return;
   }
}

int svc_branch(void *helper, char *branch_name)
{
   struct svc *data = (struct svc *)helper;
   int statusValid = 0;

   // if branch name is nothing return nothing
   if(branch_name == NULL)
   {
     return -1;
   }
   // if the branch name is already used return -2
   for(int i =0; i < data->countBranches; i++ )
   {
     if(strcmp(data->ListBranches[i]->name,branch_name)==0)
     {


       return -2;
     }
   }
   //uncommited changes
   int numCount = 0;
   for(int j = 0; j < data->countFile; j++)
   {
     numCount += (data->ListOfFiles[j]->statusTracked);
   }
   if(numCount != data->countFile){
     return -3;
   }
   //if the branch name is invalid

   for(int i =0; i < strlen(branch_name);i++)
   {
     if(isalpha(branch_name[i]) || isdigit(branch_name[i]) || branch_name[i] == '-' || branch_name[i] == '/'|| branch_name[i] == '_')
     {
       statusValid +=1;
     }
     else
     {
       statusValid = 0;
     }

   }
   if(statusValid <strlen(branch_name))
   {

     return -1;
   }
   // else construct the new branch which is a copy of the active branch
   struct branches *b = malloc(sizeof(struct branches)*5);
   b->listOfCommits = malloc(sizeof(struct commit)*10);
   data->ListBranches = realloc(data->ListBranches,sizeof(struct branches)*10);
   data->ListBranches[data->countBranches] = b;
   data->ListBranches[data->countBranches]->name = malloc(sizeof(char)*50);
   strcpy(data->ListBranches[data->countBranches]->name,branch_name);
   for(int i = 0; i < data->active->commitCount; i++)
   {
     b->listOfCommits[i] = data->active->listOfCommits[i];
   }
   data->countBranches +=1;
   b->commitCount = data->active->commitCount;
   b->HeadCommit = data->active->HeadCommit;
   return 0;
}

int svc_checkout(void *helper, char *branch_name) {
   struct svc *data = (struct svc *)helper;
   int result = 0;
   if(branch_name == NULL)
   {
     return -1;
   }
   for (int i = 0 ; i < data->countBranches; i++)
   {
     if(strcmp(data->ListBranches[i]->name,branch_name)==0)
     {
       result =1;
     }
   }
   if(result == 0)
   {
     return -1;
   }

   // uncommited change
   int numCount = 0;
   for(int j = 0; j < data->countFile; j++)
   {
     numCount += (data->ListOfFiles[j]->statusTracked);
   }
   if(numCount != data->countFile){
     return -2;
   }

   // make active
   for (int i = 0 ; i < data->countBranches; i++)
   {
     if(strcmp(data->ListBranches[i]->name,branch_name)==0)
     {
       data->active = data->ListBranches[i];
     }
   }


   return 0;

}

char **list_branches(void *helper, int *n_branches)
{
 struct svc *data = (struct svc *)helper;
 char **list_branch = (char**) malloc(20 * sizeof(char*));
 char *b = NULL;

   if(n_branches !=NULL)
   {
     (*n_branches) = data->countBranches;
     //for all branches under SVC
     for(int i = 0; i < data->countBranches; i++)
     {
       b = data->ListBranches[i]->name;
       list_branch[i] = b;
       printf("%s\n",data->ListBranches[i]->name);
     }

     return list_branch;
   }
   return NULL;
}

int svc_add(void *helper, char *file_name)
{
   struct svc *data = (struct svc *)helper;
   int hashVal = hash_file(helper,file_name);
   if(file_name == NULL)
   {
     return -1;
   }
   else if(hashVal == -2)
   {
     return -3;
   }

   for(int i = 0; i < data->countFile; i++)
   {
     // you set the removed to 1 is manually removed
     if(hash_file(helper, data->ListOfFiles[i]->name) == -2)
     {
       data->ListOfFiles[i]->removed =1;
     }
     if(strcmp(data->ListOfFiles[i]->name,file_name)==0 && data->ListOfFiles[i]->removed == 0  )
     {
       return -2;
     }
   }
   addFile(helper, file_name, hashVal);
   return hashVal;
}


int svc_rm(void *helper, char *file_name)
{
    if(file_name == NULL)
    {
      return -1;
    }

    struct svc *data = (struct svc *)helper;
    for(int i = 0; i < data->countFile;  i++)
    {
      if(data->active->HeadCommit != NULL)
      {
        for(int j = 0; j < data->active->HeadCommit->filesTracked;j++)
        {
          if(strcmp(data->active->HeadCommit->tracked[j].name,file_name ) ==0)
          {
            data->active->HeadCommit->tracked[j].removed =1;
          }
        }
      }

      if(strcmp(data->ListOfFiles[i]->name,file_name )== 0 && (data->ListOfFiles[i]->removed ==0 ))
      {
        int hash = data->ListOfFiles[i]->hash_value;
        data->ListOfFiles[i]->removed =1;
        return hash;
      }
    }
    return -2;
}

int svc_reset(void *helper, char *commit_id) {
   int status = 0;
   struct svc *data = (struct svc *)helper;
   if(commit_id == NULL)
   {
     return -1;
   }
   for(int i = 0; i < data->active->commitCount; i++)
   {
     if(strcmp(data->active->listOfCommits[i].commitId,commit_id)==0){
       status =1;
     }
   }
   if(status == 0)
   {
     return -2;
   }

   int position = 0;
   for(int i = 0; i < data->active->commitCount; i++)
   {
     if(strcmp(data->active->listOfCommits[i].commitId, commit_id)==0)
     {
       data->active->HeadCommit = &data->active->listOfCommits[i];
       position =i;
       break;
     }
   }
   for(int j = position+1 ; j < data->active->commitCount; j++)
   {
     // remove the commits after the reset one
     data->active->listOfCommits[j].removed =1;

   }
   for(int k = 0; k< data->active->listOfCommits[position].filesTracked;k++)
   {
     data->active->listOfCommits[position].tracked[k].removed = 0;
     for(int j = 0; j< data->countFile; j++){
       data->ListOfFiles[j]->removed = 0;
       if (strcmp(data->ListOfFiles[j]->name,data->active->listOfCommits[position].tracked[k].name)==0)
       {
         //restored back
         data->ListOfFiles[j]->removed = 0;
         break;
       }
     }
   }
   //not tracked removed
   for(int k = 0; k < data->countFile; k++)
   {
     if(data->ListOfFiles[k]->statusTracked == 0)
     {
       data->ListOfFiles[k]->removed =1;
     }
   }
    return 0;
}




char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {
//     // TODO: Implement
   struct svc *data = (struct svc *)helper;
   int status = 0;
   if(branch_name == NULL)
   {
     printf("Invalid branch name\n");
     return NULL;
   }
   for(int i =0; i < data->countBranches;i++)
   {
     if(strcmp(data->ListBranches[i]->name,branch_name)==0)
     {
       status = 1;
       if(strcmp(data->active->name,branch_name)==0)
       {
         printf("Cannot merge a branch with itself\n");
         return NULL;
       }
     }
   }
   if(status == 0)
   {
     printf("Branch not found\n");
     return NULL;
   }
   //Branch merge create message
   data->active->merged =1;
   char *message= malloc(sizeof(char)*50);
   strcpy(message,"Merged branch ");
   strcat(message,branch_name);

   int commitId = 0;
   int placement = 0;

   for(int m = 0; m < data->countBranches; m++)
   {
     //find branch position in svc
     if(strcmp(data->ListBranches[m]->name,branch_name)==0)
     {
       placement = m;
     }
   }

   for(int k = 0; k < data->ListBranches[placement]->HeadCommit->filesTracked; k++)
   {
     //check that all changes are commited
     if(hash_file(helper, data->ListBranches[placement]->HeadCommit->tracked[k].name) != data->ListBranches[placement]->HeadCommit->tracked[k].hash_value)
     {

       free(message);
       printf("Changes must be committed\n");
       return NULL;
     }
   }

   struct commit *c = malloc(sizeof(struct commit));
   c->tracked = malloc(sizeof(struct files)*10);
   c->filesTracked = 0;
   c->branch_under = malloc(sizeof(char)*20);
   strcpy(c->branch_under,data->active->name);


   for(int j = 0; j < strlen(message); j++)
   {
     commitId = (commitId + message[j]) % 1000;
   }
   //make new commit and add;
   for(int i = 0; i < n_resolutions; i++)
   {
     if(resolutions[i].resolved_file != NULL)
     {
       //if in resolution add
       commitId = commitId + 9573681;
       svc_add(helper,resolutions[i].resolved_file);
       c->tracked[c->filesTracked] = *data->ListOfFiles[data->countFile-1];
       c->tracked[c->filesTracked].name = malloc(sizeof(char)*20);
       strcpy(c->tracked[c->filesTracked].name,resolutions[i].file_name);
       c->filesTracked +=1;
       for(int j = 0; j< strlen(resolutions[i].file_name); j++)
       {
         commitId = (commitId * (resolutions[i].file_name[j] %37)) % 15485863 + 1;
       }
     }
     else
     {
       commitId = commitId + 85973;
       for(int j = 0; j< strlen(resolutions[i].file_name); j++)
       {
         commitId = (commitId * (resolutions[i].file_name[j] %37)) % 15485863 + 1;
       }
       for(int p= 0; p<data->ListBranches[placement]->HeadCommit->filesTracked;p++ )
       {
         if(strcmp(data->ListBranches[placement]->HeadCommit->tracked[p].name,resolutions[i].file_name)==0)
         {
           data->ListBranches[placement]->HeadCommit->tracked[p].removed =1;
         }
       }
     }
   }



   int found = 0;

   for(int i = 0; i < data->ListBranches[placement]->HeadCommit->filesTracked; i++)
   {
     for(int j = 0; j < data->active->HeadCommit->filesTracked; j++)
     {
       if(strcmp(data->active->HeadCommit->tracked[j].name,data->ListBranches[placement]->HeadCommit->tracked[i].name)==0)
       {
         found =1;
       }
     }
     if(found ==0 && data->ListBranches[placement]->HeadCommit->tracked[i].removed != 1)
     {

       c->tracked[c->filesTracked] = data->ListBranches[placement]->HeadCommit->tracked[i];
       c->tracked[c->filesTracked].name = malloc(sizeof(char)*20);
       strcpy(c->tracked[c->filesTracked].name,data->ListBranches[placement]->HeadCommit->tracked[i].name);
       c->filesTracked +=1;
       // printf("name: %s\n",data->ListBranches[placement]->HeadCommit->tracked[i].name);
       data->ListBranches[placement]->HeadCommit->tracked[i].new_merged =1;

       commitId= commitId + 376591;
       for(int k = 0; k < strlen(data->ListBranches[placement]->HeadCommit->tracked[i].name); k++)
       {
         commitId = (commitId * (data->ListBranches[placement]->HeadCommit->tracked[i].name[k] %37)) % 15485863 + 1;
       }

     }
     found = 0;

   }


   //constructing commit
   c->removed = 0;
   c->merged = 1;

    c->commitId = malloc(sizeof(char)*40);
   sprintf(c->commitId,"%06x",commitId);
   c->message =malloc(sizeof(char)*50);
   strcpy(c->message, message);
   c->parent = malloc(sizeof(struct commit*)*10);
   data->active->listOfCommits = realloc(data->active->listOfCommits,sizeof(struct commit)*10);
   //adding the previous branch parent commit before adding active parent commit
   struct commit *last = &data->ListBranches[placement]->listOfCommits[data->ListBranches[placement]->commitCount-1];
   //add file in
   for (int i = last->filesTracked-1; i >=0; i--)
   {

     c->tracked[c->filesTracked] = last->tracked[i];
     last->tracked[i].statusTracked = 1;
     c->tracked[c->filesTracked].name = malloc(sizeof(char)*20);
     strcpy(c->tracked[c->filesTracked].name,last->tracked[i].name);
     c->filesTracked +=1;


   }
   data->active->listOfCommits[data->active->commitCount] = *last;
   data->active->commitCount +=1;
   data->active->listOfCommits[data->active->commitCount] = *c;
   data->active->commitCount +=1;
   data->ListOfCommits = realloc(data->ListOfCommits,sizeof(struct commit*)*10);
   data->ListOfCommits[data->TotalCommit] = c;
   data->TotalCommit +=1;
   (*c->parent) = *data->active->HeadCommit;
   data->active->HeadCommit = c;
   free(message);
   printf("Merge successful\n");
   return c->commitId;
}
