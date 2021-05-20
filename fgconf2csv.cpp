//@author Javier Alfonso 
//@since apr 2020
//fortigate convert configuration to csv

#include<fstream>
#include<iostream>
#include<string>

using namespace std;

struct attribute{
  string name;
  string value;
  attribute * next=NULL;
};

struct object{
  string parent;
  string name;
  attribute* list=NULL;
  bool config;
  object* sons=NULL;
  object * next=NULL;
}; 

struct row{
  string data;
  row *next=NULL;
};

struct list{
  row * columns=NULL;
  list * next=NULL;
};


string rmspacesbf(char *line){
  int i=0;
  int tab=0;
  int len=0;
  bool first=false;
  string text;
  while(line[i]!='\n'){
    if(line[i]==' '&& !first)tab++; else first=true; 
    i++;  
  }
  string other=line;
  if(tab>0){
    return other.substr(tab,i-tab);
  }
  else {
    return other.substr(0,i);
  }
}

void create_objects(row ** source, object ** appliances){
  bool exit=false;
  while((*source)!=NULL&&!exit){
    size_t  poscom=(*source)->data.find(' ');
    string command;
    if(poscom!=-1)command=(*source)->data.substr(0,poscom);else command=(*source)->data;
    if((command.compare("config"))==0||(command.compare("edit"))==0){
        object * son=new object;
        son->parent=(*appliances)->name;
        string name=(*source)->data.substr(poscom,(*source)->data.length());
        son->name=name;
        son->config=((command.compare("config"))==0);
        *source=(*source)->next;
        create_objects(source,&son);
        if((*appliances)->sons!=NULL){
          object * last=(*appliances)->sons;
          while(last->next!=NULL){last=last->next;}
          last->next=son;
        }else{
          (*appliances)->sons=son;
        }
    }else if((command.compare("end"))==0||(command.compare("next"))==0){
       exit=true;
       *source=(*source)->next;
    }else if((command.compare("set"))==0){
        attribute * son=new attribute;
        size_t  posname=(*source)->data.find(' ',poscom+1);
        string name=(*source)->data.substr(poscom+1,posname-poscom-1);
        string value=(*source)->data.substr(posname+1,(*source)->data.length()-posname);
        son->name=name;
        son->value=value;       
        if((*appliances)->list!=NULL){
          attribute * last=(*appliances)->list;
          while(last->next!=NULL){last=last->next;}
          last->next=son;
        }else{
          (*appliances)->list=son;
        } 
         *source=(*source)->next;
    }else{
        *source=(*source)->next;
    }
  }
}

void create_lists(object ** appliances, list ** lists){
  object * parent=(*appliances);
  while(parent!=NULL){
    if(parent->sons!=NULL){
      if(parent->config){
        list * listnode=new list;
        object * son=parent->sons;
        row * titles=new  row;
        titles->data="########################################################################### ";
        titles->data=titles->data+parent->name;
        titles->data=titles->data+" ###########################################################################";
        row * values=NULL;
        bool repeated=false;
        list * other;
        listnode->columns=titles;
        if((*lists)==NULL){
          (*lists)=listnode;
        }else{
          other=*lists;
          while(other->next!=NULL){other=other->next;}
          other->next=listnode;
          other=other->next;
        }     
        listnode->next=new list;
        listnode=listnode->next;
        other=listnode;   
        titles=new row;
        titles->data="Name";
        listnode->columns=titles;
        while(son!=NULL){
          attribute * column=son->list;
          while(column!=NULL){
            titles=listnode->columns;
            repeated=false;
            while(!repeated && titles!=NULL){
              repeated=(titles->data.compare(column->name)==0);
              titles=titles->next;
            }
            if(!repeated){
              titles=listnode->columns;
              while(titles->next!=NULL){
                titles=titles->next;
              }
              titles->next=new row;
              titles=titles->next;
              titles->data=column->name;
            }
            column=column->next;
          } 
          son=son->next;
        }
        son=parent->sons;
        while(son!=NULL){
          list * listson=new list;
          titles=listnode->columns;
          attribute * column=son->list;
          listson->columns=new row;
          values=listson->columns;
          values->data=son->name;
          titles=titles->next;
          while(titles!=NULL){
            values->next=new row;
            values=values->next;
            repeated=false;
            column=son->list;
            while(!repeated && column!=NULL){
              repeated=(titles->data.compare(column->name)==0);
              if(repeated){
                values->data=column->value;
              }
              column=column->next;
            }
            titles=titles->next;
          }
          other->next=listson;
          other=other->next;
          son=son->next;
        }
      }
      create_lists(&(parent->sons),lists);
    }
    parent=parent->next;
  } 
}


void delete_tree(object ** appliances){
  object * parent=*appliances;
  while(parent!=NULL){
    attribute * last;
    while(parent->list!=NULL){
      last=parent->list;
      parent->list=last->next;
      delete last;
    }
    while(parent->sons!=NULL){
      object * son=parent->sons;
      parent->sons=son->next;
      delete_tree(&son);
      delete son;
     }
    parent=parent->next; 
  }
}

void print_tree(object ** appliances){
 object * parent=(*appliances);
 while(parent!=NULL){
    cout<<"Object name:"<<parent->name<<endl;
    cout<<"Object parent:"<<parent->parent<<endl;
    attribute * payload=parent->list;
    cout<<"--> Attribute:Value"<<endl;
    while(payload!=NULL){
      cout<<"-->"<<payload->name<<":"<<payload->value<<endl;
      payload=payload->next;
    }
    if(parent->sons!=NULL){ 
      print_tree(&parent->sons);
    }
    parent=parent->next;
  }
}

int main (int argc, char **argv){
  FILE* fin;
  FILE* fout;
  char* line;
  size_t len=0;
  object * appliances=NULL;
  row * source=NULL;
  int size=0; 
  list * lists=NULL;
  if(argc!=3){
    cout<<"usage '"<<string(argv[0])<<" <conf_file> <out_file>'"<<endl;
    return 0;
   }
  fin=fopen(argv[1],"r");
  if(fin==NULL){
    cout<<"error reading conf_file"<<endl;
    return -1;
  }
  fout=fopen(argv[2],"w");
  if(fout==NULL){
    cout<<"error writing out_file"<<endl;
    return -1;
  }
  cout<<"extracting data"<<endl;
  row * node=NULL; 
  while((getline(&line,&len,fin))!=-1){
    string text=rmspacesbf(line);
    if(line[0]!='#'){
      if(source==NULL){
        source=new row;
        node=source;
      }
      else{
        node->next=new row;
        node=node->next; 
      }
      node->data=text;
    }
  }
  fclose(fin);
  cout<<"creating hierarchy tree"<<endl;
  row * work=source;
  appliances=new object;
  appliances->name="root";
  object * tree=appliances;
  create_objects(&work,&tree);
  cout<<"creating lists"<<endl;
  tree=appliances;
  //print_tree(&tree);
  create_lists(&tree,&lists);
  cout<<"exporting data"<<endl;
  list * lines=lists;
  while(lines!=NULL){
    row * column=lines->columns;
    while(column!=NULL){
      fprintf(fout,"%s;",column->data.c_str());
      column=column->next;
    }
    fprintf(fout,"\n");
    lines=lines->next;
  }
  fclose(fout);
  cout<<"deleting objects"<<endl;
  while(source!=NULL){
    node=source;
    source=source->next;
    delete node;
  }
  list * garbage=NULL;
  row * trash=NULL;
  while(lists!=NULL){
    row * column=lists->columns;
    while(column!=NULL){
      trash=column;
      column=column->next;
      delete trash;
    }
    garbage=lists;
    lists=lists->next;
    delete garbage;
  }
  delete_tree(&appliances);
  while (appliances!=NULL){
    tree=appliances;
    appliances=appliances->next;
    delete tree;
  }
  return 0;
}
