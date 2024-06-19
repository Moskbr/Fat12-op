#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "fat12.h"

void printFat12Info(boot_sector* p){
    // the start of floopy disk is exactly the header

    char buffer[12];
    
    printf("Sector_Size:      %u\n", p->sector_size);
    printf("SecPerClus:       %u\n", p->sector);
    printf("RsvdSecCnt:       %u\n", p->N_reserved);
    printf("NumFATs:          %u\n", p->N_FATs);
    printf("Nmax_RootEntCnt:  %u\n", p->Nmax_rootEntries);
    printf("qtd_Sectors:      %u\n", p->qtd_sectors);
    printf("sec_FATSz16:      %u\n", p->secFAT);
    printf("SecPerTrack:      %u\n", p->secTrack);
    printf("NumHeads:         %u\n", p->N_heads);
    printf("BootSig:          0x%02x\n", p->boot_signature);
    printf("VolID:            %u\n", p->volume_id);

    memcpy(buffer, p->volume_label, 11);
    buffer[11] = '\0';
    printf("VolLabel:         %s\n", buffer);

    memcpy(buffer, p->type, 8);
    buffer[8] = '\0';
    printf("FileSysType:      %s\n", buffer);
}

void readDISK(char* file_name, fat12* f){
    FILE* DISK = fopen(file_name, "rb");
    if(!DISK){
        printf("Erro ao ler o disco\n");
        exit(-1);
    }
    int tam;
    tam = fread(f->disco,sizeof(fat12), 1, DISK);
    fclose(DISK);
}

void read_BootSector(fat12* f){
    // p->sector_size = 0;
    // p->sector = 0;
    // p->N_reserved = 0;
    // p->N_FATs = 0;
    // p->Nmax_rootEntries = 0;
    // p->qtd_sectors = 0;
    // p->secFAT = 0;
    // p->secTrack = 0;
    // p->N_heads = 0;
    // p->secCount = 0;
    // p->boot_signature = 0;
    // p->volume_id = 0;
    // memcpy(p->volume_label,0,11);
    // memcpy(p->type,0,8);

    p = (boot_sector*)f->disco;
    char slash[] = "/";
    memcpy(p->volume_label, slash, 1);
    //printFat12Info(p);
}

void readFAT_table(fat12* f){
    char* FAT1 = f->disco + p->sector_size;// pulando o bootSector
    int fat_entries = ceil(p->sector_size*8/1.5);// 512*8 bytes dividos em 12bits=1.5bytes
    FAT_table = (short*)malloc(sizeof(short)*fat_entries);

    char buffer[3];// para ler 3 bytes por vez e converter em 2 entradas de 12 bits
    int j, i=0;
    int index = p->N_reserved*p->sector_size;
    while(i < fat_entries/2){// 512*8 bytes divididos em 3 bytes => fat_entries/2
        memcpy(buffer,(char*)(f->disco + index),3);
        index += 3;
        // utilizando 0x0F e 0xF0 como mascaras para converter 3 bytes em 2 de 12bits
        // considerando arquitetura little-endian
        short first_entry = 0;
        first_entry = (buffer[0] | ((buffer[1] & 0x0F)<<8));// FE = (b[1].metade-dir)<<8 + b[0]
        short second_entry = 0;
        second_entry = (((buffer[1] & 0xF0) >> 4) | (buffer[2] << 4));// SE = b[2]<<4 + (b[1].metade.esq)>>4

        FAT_table[2*i] = first_entry;
        FAT_table[2*i+1] = second_entry;
        i++;
    }
}

void read_RootDirectory(fat12* f){
    int root_offset = (p->N_reserved + (p->N_FATs*p->secFAT))*p->sector_size;
    int i;
    root_directory* r = (root_directory*)malloc(sizeof(root_directory));
    root_vector = (root_directory*)malloc(sizeof(root_directory)*p->Nmax_rootEntries);
    int Nmax_rootEntries_inBytes = 32*p->Nmax_rootEntries;
    for(i=0;(i*32)<Nmax_rootEntries_inBytes; i++){
        int index = root_offset + i*32;
        r = (root_directory*)(f->disco + index);
        memcpy(root_vector[i].fileName, r->fileName, 8);
        root_vector[i].fileName[7] = '\0';
        memcpy(root_vector[i].extension, r->extension, 3);
        root_vector[i].attributes = r->attributes;
        root_vector[i].creation_time = r->creation_time;
        root_vector[i].creation_date = r->creation_date;
        root_vector[i].first_cluster = r->first_cluster;
        root_vector[i].fileSize = r->fileSize;
        root_vector[i].LAD = r->LAD;
        root_vector[i].LWD = r->LWD;
        root_vector[i].LWT = r->LWT;
    }
}

void read_Subdirectories(fat12* f){
    int i, j=0;
    s = NULL;
    for(i=0;i<p->Nmax_rootEntries; i++){
        if((root_vector[i].attributes & 0x10) && (root_vector[i].fileName[0]!=0xE5) && (root_vector[i].fileName[0]!=0x00)){
            char caminho[20] = "/";
            strcat(caminho,root_vector[i].fileName);
            s = new_SubDir(s,i,caminho);

            // as duas primeiras entradas sao '.' e '..' (aka estruturas 'root_directory') de 32 bytes cada
            int sub_offset = (33 + root_vector[i].first_cluster - 2)*p->sector*p->sector_size + 58;

            // pula-se mais 58 bytes para obtermos parent_cluster da entrada '..'
            s->parent_cluster = (short)f->disco[sub_offset];
            sub_offset += 6;// pula bytes restantes, para se obter as demais entradas

            // obtendo as demais entradas
            for(j=0; j<MAX_FILES_SUDIR; j++){

                s->dir[j].first_cluster = root_vector[i].first_cluster;

                strncpy(s->dir[j].fileName,(char*)(f->disco+sub_offset),8);
                sub_offset += 8;

                strncpy(s->dir[j].extension, (char*)(f->disco+sub_offset),3);
                sub_offset += 3;

                if(s->dir->fileName[0] == 0xE5 || s->dir->fileName[0] == 0x00){
                    sub_offset += (32-8-3);
                    continue;
                }

                s->dir[j].attributes = f->disco[sub_offset];
                sub_offset++;
                s->dir[j].reserved = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].creation_time = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].creation_date = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].LAD = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].ignore = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].LWT = (short)f->disco[sub_offset];
                sub_offset += 2;
                s->dir[j].LWD = (short)f->disco[sub_offset];
                sub_offset += 2;
                sub_offset += 2;// ignore
                s->dir[j].fileSize = (int)f->disco[sub_offset];
                sub_offset += 4;
            }
        }
    }
}

void printCommands(){
    printf("\nOpcoes de comandos:\n");
    printf(">> ls                    -- listagem de todos os arquivos diretorios do sistema\n");
    printf(">> ls-1                  -- listagem apenas do diretorio raiz\n");
    printf(">> grab [\"File Path\"]    -- Copia de um arquivo do disco rigido para o sistema\n");
    printf(">> cp   [\"File Name\"]    -- Copia de um arquivo do sistema para o disco rigido\n");
    printf(">> q                     -- Encerrar\n");
    printf(">> ?                     -- Mostrar opcoes de comandos\n");
    printf("\n>> ");
}

void cmd_ls1(){
    int i, j;
    for(i=0;i<p->Nmax_rootEntries; i++){
        if(root_vector[i].fileName[0] == 0xE5 || root_vector[i].fileName[0] == 0x00) continue;
        printf("/");
        if(root_vector[i].attributes & 0x10){
            printf("%s      (diretorio)\n", root_vector[i].fileName);
            continue;
        }
        char fname[8];
        memcpy(fname, (char*)(root_vector+i)->fileName, 8);
        for(j=0;fname[j] != ' '; j++) ;
        fname[j] = '\0';
        printf("%s.%s\n",fname, root_vector[i].extension);
    }
}

subdirectory* new_SubDir(subdirectory* s, int r_index, char* cam){
    subdirectory* novo = (subdirectory*)malloc(sizeof(subdirectory));
    novo->dir = (root_directory*)calloc(MAX_FILES_SUDIR,sizeof(root_directory));
    novo->path = (char*)malloc(sizeof(char)*20);
    novo->root_index = r_index;
    strncpy(novo->path,cam,20);
    novo->next = s;
    return novo;
}

void cmd_ls(){
    printf("\nRoot:\n");
    cmd_ls1();

    printf("\nSubdiretorios:\n");
    int i, j;
    subdirectory* index = s;
    while(index != NULL){
        for(j=0; j<MAX_FILES_SUDIR; j++){
            if(index->dir[j].fileName[0] == 0xE5 || index->dir[j].fileName[0] == 0x00){
                break;
            }
            if(index->dir[j].attributes & 0x10){
                printf("%s/%s      (diretorio)\n", index->path, index->dir[j].fileName);
                break;
            }
            char fname[8];
            memcpy(fname, index->dir[j].fileName, 8);
            for(i=0;fname[i] != ' '; i++) ;
            fname[i] = '\0';
            printf("%s/%s.%s\n",index->path, fname, index->dir[j].extension);
        }
        index = index->next;
    }
    
}

void free_subdir(){
    if(s->next != NULL) free_subdir(s->next);
    free(s);
}

void grab(fat12* f, char* file_path){
    char target = ' ';// inicializacao
    int i, dist=0;

    // transforma em letras maiusculas
    for(i=0; i<strlen(file_path); i++){
        file_path[i] = toupper(file_path[i]);
    }

    for(i=strlen(file_path); i>=0 && target != '/'; i--, dist++)
        target = file_path[i];// procura a ultima '/'
    
    char complete_name[dist+1+1];
    strncpy(complete_name, (file_path+i+2), dist);// obtem o nome completo do arquivo
    complete_name[dist+1] = '\0';

    int N = dist;
    for(i=0; i<N && target != '.'; i++)
        target = complete_name[i];// procura o ponto que separa o nome do arquivo da extensao
    
    int dot_pos = i-1;
    char ext[4];
    char fname[8];
    int j=0;
    for(i=0;i<N && complete_name[i] != '\0';i++){// obtem-se o nome e a extensao do arquivo
        if(i < dot_pos) fname[i] = complete_name[i];
        if(i == dot_pos) {fname[i] = ' '; ext[j+1] = ' ';}
        if(i > dot_pos) ext[j++] = complete_name[i];
    }

    for(i=dot_pos;i<7;i++) fname[i] = ' ';// fname sem \0 igual ao dir->fileName
    for(i=0;i<3;i++) if(!ext[i]) ext[i] = ' ';
    ext[3] = '\0';
    fname[7] = '\0';
    char file_name[12] = "";
    strcat(file_name, fname);
    strcat(file_name, ext);
    file_name[11] = '\0';

    // agora obtendo o diretorio do arquivo
    for(i=strlen(file_path); i>=0 && file_path[i] != '/'; i--) ;// acha a ultima /

    int T = i;
    char dir[8];
    for(i=1;i<T; i++) dir[i-1] = file_path[i];// obtendo diretorio
    dir[7] = '\0';
    for(i=0;i<7;i++) if(!dir[i]) dir[i] = ' ';
    if(T == 0) dir[0] = '\0';

    // cria arquivo de escrita
    char output[50] = "C:\\Users\\LUIZF\\Documents\\";
    // reestruturacao do nome original
    char fout[8] = "";
    char eout[4] = "";
    strncpy(fout, fname, 8);
    for(i=0;i<8;i++) if(fout[i] == ' ') {fout[i] = '\0'; break;}
    strncpy(eout, ext, 4);
    for(i=0;i<3;i++) if(eout[i] == ' ') {eout[i] = '\0'; break;}
    char fullname[11] = "";
    strcat(fullname, fout);
    strcat(fullname, ".");
    strcat(fullname, eout);
    fullname[10] = '\0';


    strcat(output, fullname);
    FILE* OUT = fopen(output, "w");// cria arquivo
    if(!OUT){
        printf("Erro ao criar arquivo\n");
        exit(-2);
    }
    
    if(dir[0] == '\0'){// se o diretorio for root
        // pegar arquivo e escreve-lo (copia-lo) fora do disco: copiar dados -> sequencia de clusters
        int i;
        for(i=0; i<p->Nmax_rootEntries; i++){
            if(root_vector[i].fileName[0] != 0xE5 && root_vector[i].fileName[0] != 0x00){
                root_directory entry;
                char temp[12] = "";
                strcat(temp, root_vector[i].fileName);
                strcat(temp, root_vector[i].extension);
                temp[11] = '\0';
                if(!strcmp(temp, file_name)){
                    entry = root_vector[i];
                    copy_data(f, entry, OUT);
                    break;
                }
            }
        }
    }
    else{
        for(i=0; i<p->Nmax_rootEntries; i++){
            if(!strcmp(dir, root_vector[i].fileName) && (root_vector[i].attributes & 0x10)){
                while(s->root_index != i) s = s->next;
                int j=0;
                char temp[12] = "";
                for(; j<MAX_FILES_SUDIR; j++){
                    strcat(temp, s->dir[j].fileName);
                    strcat(temp, s->dir[j].extension);
                    temp[11] = '\0';
                    if(!strcmp(temp, file_name)){
                        //root_directory entry;
                        //entry = s->dir[j];
                        copy_data(f, s->dir[j], OUT);
                        break;
                    }
                }
            }
        }
    }

    fclose(OUT);
}

void copy_data(fat12* f, root_directory r, FILE* Dest){
    int cur_cluster = r.first_cluster;
    char buffer[p->sector*p->sector_size];// [bytes per sector]

    while(cur_cluster < 0xF8){
        int offset = (33 + cur_cluster - 2)*p->sector*p->sector_size;
        memcpy(buffer, (char*)(f->disco + offset), sizeof(buffer));
        int min = r.fileSize <= (p->sector*p->sector_size) ? r.fileSize : (p->sector*p->sector_size);
        fwrite(buffer, min, 1, Dest);
        cur_cluster = FAT_table[cur_cluster];
    }
}

void cp(fat12* f, char* fsource){
    int i, N;

    for(i=0; i<N && fsource[i] != '.'; i++);
    // procura o ponto que separa o nome do arquivo da extensao
    int dot_pos = i-1;
    char ext[4];
    char fname[8];
    int j=0;
    for(i=0;i<N && fsource[i] != '\0';i++){// obtem-se o nome e a extensao do arquivo
        if(i < dot_pos) fname[i] = fsource[i];
        if(i == dot_pos) {fname[i] = ' '; ext[j+1] = ' ';}
        if(i > dot_pos) ext[j++] = fsource[i];
    }
    for(i=dot_pos;i<7;i++) fname[i] = ' ';// fname sem \0 igual ao dir->fileName
    for(i=0;i<3;i++) if(!ext[i]) ext[i] = ' ';
    ext[3] = '\0';
    fname[7] = '\0';

    // copiando o conteudo de fsource para o buffer
    char path[50] = "C:\\Users\\LUIZF\\Documents\\";
    strcpy(path, fsource);
    FILE* fp = fopen(path, "r");
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);// pega o tamanho do arquivo
    fseek(fp, 0, SEEK_SET);

    char* buffer = malloc(fsize + 1);
    fread(buffer, sizeof(buffer), 1, fp);
    int clusters = (int)ceil(sizeof(buffer)/(p->sector*p->sector_size));// clusters necessarios

    for(i=0;i<p->Nmax_rootEntries;i++){
        if(root_vector[i].fileName[0] == 0xE5 || root_vector[i].fileName[0] == 0x00){
            strcpy(root_vector[i].fileName, fname);
            strcpy(root_vector[i].extension, ext);
            int k;
            for(k=2;k< FAT_table[k] != 0x000; k++);
            root_vector[i].first_cluster = FAT_table[k];
            break;
        }
    }

    FILE* DISK = fopen("C:\\Users\\LUIZF\\Documents\\fat12subdir.img", "w");
    if(!DISK){
        printf("Erro ao abir o disco para escrita\n");
        exit(-3);
    }

    escreveDisco(DISK, buffer, clusters, root_vector[i].first_cluster);

    fclose(DISK);
    DISK = NULL;
    readDISK("C:\\Users\\LUIZF\\Documents\\fat12subdir.img",f);
    fclose(fp);
}

void escreveDisco(FILE* DISK, char* buffer, int clusters, int first){
    int bytes_file = sizeof(buffer);
    int bytes_disk = (int)clusters*p->sector_size;

    while(first < 0xFF8 && bytes_file > 0){
        int data_offset = ((33 + first - 2)*p->sector*p->sector_size);
        fseek(DISK, data_offset, SEEK_SET);

        int tot_bytes = bytes_file < (p->sector*p->sector_size) ? bytes_file : (p->sector*p->sector_size);
        fwrite(buffer, tot_bytes, 1, DISK);
        
        bytes_file -= tot_bytes;
        buffer = (char*)(buffer + tot_bytes);

        if(bytes_disk > 0){
            FAT_table[first] = 0xFF8;
            int k;
            for(k=2;k< FAT_table[k] != 0x000; k++);
            short new = FAT_table[k];
            FAT_table[first] = new;
            first = new;
        }
    }

    if(first < 0xFF8){
        FAT_table[first] = 0xFFF;
        if(bytes_disk - sizeof(buffer) > 0){
            fwrite(NULL, bytes_disk - sizeof(buffer), 1, DISK);
        }
    }
}

int main ()
{
    printf("\nEntre com o caminho do disco (imagem): ");
    //user: scanf("%s", file_name);
    char file_name[] = "C:\\Users\\LUIZF\\Documents\\fat12subdir.img";
    printf("%s\n", file_name);// auto

    // leitura dos setores do disco
    fat12* f;
    f = (fat12*)malloc(sizeof(fat12));
    readDISK(file_name,f);// le o disco inteiro
    read_BootSector(f);
    readFAT_table(f);
    read_RootDirectory(f);
    read_Subdirectories(f);

    //  menu de comandos
    char cmd[20];
    printCommands();
    scanf("%s", cmd);

    while(strcmp(cmd,"q")){
        if(!strcmp(cmd,"ls")){
            cmd_ls();
        }
        else if(!strcmp(cmd,"ls-1")){
            cmd_ls1();
        }
        else if(!strcmp(cmd, "grab")){
            char file_path[30];
            scanf("%s", file_path);
            grab(f, file_path);
        }
        else if(!strcmp(cmd, "cp")){
            char name_file[11];
            scanf("%s", name_file);
            //cp(f, name_file);
        }
        else if(!strcmp(cmd, "?")){
            printCommands();
        }

        printf("\nEntre com '?' para ver opcoes de comandos ou 'q' para encerrar.\n");
        printf("\n>> ");
        scanf("%s", cmd);
    }

    free(p);
    free(FAT_table);
    free(root_vector);
    free_subdir();
    return 0;
}