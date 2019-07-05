#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include "ArvoreB.h"

void aleatorios(char bolsa[20])
{
	char linha[2048]; //Cada linha é um registro
    FILE *f = fopen(bolsa,"r");// abrir o arquivo da bolsa
    FILE *s = fopen("bolsa2.csv", "w"); //criar o arquivo que conterá uma amostra do arquivo "bolsa" original

    printf("\nDigite o numero de linhas: ");
    long linhas;
    scanf("%ld", &linhas);
    srand((unsigned)time(NULL));
    long x;
    for(x = 1; x <= linhas; x++)
    {
        fseek(f,(rand()%100000),SEEK_SET);
    	fgets(linha, 2048, f); //Aqui, terminamos de ler uma linha pois a cabeca de leitura pode cair no meio de uma linha, numa posicao desconhecida.
                               //Não temos garantia de que  a cabeca de leittura irá cair exatamente no inicio ou no final de um registro, pois os registros possuem tamanho variavel.
    	fgets(linha, 2048, f); //Terminar de ler a linha.
    	if(linha == NULL)
            x--;
        else
		  fprintf(s, "%s", linha);
    }
    printf("\nCompleto\n");
    fclose(f);
    fclose(s);
}

ArvoreB_Pagina* ArvoreB_alocaPagina()
{
    int i;
    ArvoreB_Pagina* resp = (ArvoreB_Pagina*) malloc(sizeof(ArvoreB_Pagina));
    resp->quantidadeElementos = 0;
    resp->paginaEsquerda = 0;
    for(i=0; i<TAM_PAG; i++)
    {
        memset(resp->elementos[i].chave,'\0',TAM_CHAVE);
        resp->elementos[i].paginaDireita = 0;
        resp->elementos[i].posicaoRegistro = 0;
    }
    return resp;
}

void ArvoreB_desalocaPagina(ArvoreB_Pagina* p)
{
    if(p)
    {
        free(p);
    }
}

//A função a seguir realiza a indexacao do arquivo do bolsa.
ArvoreB* ArvoreB_Abre(const char* nomeArquivo)
{
    ArvoreB* resp = (ArvoreB*) 0;
    ArvoreB_Cabecalho* cabecalho = (ArvoreB_Cabecalho*) 0;
    ArvoreB_Pagina* raiz = (ArvoreB_Pagina*) 0;
    FILE* f = fopen(nomeArquivo, "r");
    if(!f)
    {
        f = fopen(nomeArquivo,"w");
        if(!f)
        {
            fprintf(stderr,"Arquivo %s não pode ser criado\n", nomeArquivo);
            return resp;
        }
        cabecalho = (ArvoreB_Cabecalho*) malloc(sizeof(ArvoreB_Cabecalho));
        cabecalho->raiz = sizeof(ArvoreB_Cabecalho);
        fwrite(cabecalho,sizeof(ArvoreB_Cabecalho),1,f);
        raiz = ArvoreB_alocaPagina();
        fwrite(raiz,sizeof(ArvoreB_Pagina),1,f);
        ArvoreB_desalocaPagina(raiz);
        free(cabecalho);
    }
    fclose(f);
    f = fopen(nomeArquivo, "rb+");
    resp = (ArvoreB*) malloc(sizeof(ArvoreB));
    resp->f = f;
    resp->cabecalho = (ArvoreB_Cabecalho*) malloc(sizeof(ArvoreB_Cabecalho));
    fread(resp->cabecalho,sizeof(ArvoreB_Cabecalho),1,f);
    return resp;
}

int ArvoreB_Compara(const void *e1, const void* e2)
{
    return strncmp(((ArvoreB_Elemento*)e1)->chave,((ArvoreB_Elemento*)e2)->chave,TAM_CHAVE); 
}

void ArvoreB_escreveCabecalho(ArvoreB* arvore)
{
    fseek(arvore->f, 0, SEEK_SET);
    fwrite(arvore->cabecalho, sizeof(ArvoreB_Cabecalho), 1, arvore->f);
}

void ArvoreB_Fecha(ArvoreB* arvore)
{
    if(arvore)
    {
        ArvoreB_escreveCabecalho(arvore);
        fclose(arvore->f);
        free(arvore->cabecalho);
        free(arvore);
    }
}

ArvoreB_Elemento* ArvoreB_Split(ArvoreB *arvore, long posicaoPagina, ArvoreB_Pagina* pagina, ArvoreB_Elemento* overflow)
{
    int i;
    ArvoreB_Elemento aux;
    ArvoreB_Elemento* resp;
    ArvoreB_Pagina* paginaSplit;
    // O elemento na área de overflow é menor que o ultimo elemento da página?
    if(ArvoreB_Compara(overflow,&pagina->elementos[pagina->quantidadeElementos-1])<0)
    {
        // Trocar o último com o elemento no overflow e reordenar a página.
        aux = *overflow;
        *overflow = pagina->elementos[pagina->quantidadeElementos-1];
        pagina->elementos[pagina->quantidadeElementos-1] = aux;
        qsort(pagina->elementos,pagina->quantidadeElementos,sizeof(ArvoreB_Elemento),ArvoreB_Compara);
    }
    paginaSplit = ArvoreB_alocaPagina();
    for(i=TAM_PAG/2+1;i<TAM_PAG;i++)
    {
        paginaSplit->elementos[paginaSplit->quantidadeElementos] = pagina->elementos[i];
        paginaSplit->quantidadeElementos++;
    }
    paginaSplit->elementos[paginaSplit->quantidadeElementos] = *overflow;
    paginaSplit->quantidadeElementos++;
    pagina->quantidadeElementos = TAM_PAG/2;
    fseek(arvore->f, posicaoPagina, SEEK_SET);
    fwrite(pagina, sizeof(ArvoreB_Pagina),1,arvore->f);
    fseek(arvore->f,0,SEEK_END);
    resp = (ArvoreB_Elemento*) malloc(sizeof(ArvoreB_Elemento));
    paginaSplit->paginaEsquerda = pagina->elementos[TAM_PAG/2].paginaDireita;
    memcpy(resp->chave,pagina->elementos[TAM_PAG/2].chave,TAM_CHAVE);
    resp->posicaoRegistro = pagina->elementos[TAM_PAG/2].posicaoRegistro;
    resp->paginaDireita = ftell(arvore->f);
    fwrite(paginaSplit, sizeof(ArvoreB_Pagina),1,arvore->f);
    ArvoreB_desalocaPagina(paginaSplit);
    return resp;
}

ArvoreB_Elemento* ArvoreB_Insere_Recursiva(ArvoreB* arvore, long posicaoPagina, char chave[TAM_CHAVE], long posicaoRegistro)
{
    int i;
    long posicaoPaginaFilho;
    ArvoreB_Elemento* resp = (ArvoreB_Elemento*) 0;
    ArvoreB_Elemento *elementoSplit = (ArvoreB_Elemento*) 0;
    ArvoreB_Elemento overflow;
    // printf("Inserindo %.8s na página %ld\n", chave, posicaoPagina);
    ArvoreB_Pagina *pagina = ArvoreB_alocaPagina();
    fseek(arvore->f, posicaoPagina, SEEK_SET);
    fread(pagina, sizeof(ArvoreB_Pagina),1,arvore->f);
    if(pagina->paginaEsquerda == 0) // Folha
    {
        if(pagina->quantidadeElementos < TAM_PAG)
        {
            memcpy(pagina->elementos[pagina->quantidadeElementos].chave,chave,TAM_CHAVE);
            pagina->elementos[pagina->quantidadeElementos].posicaoRegistro = posicaoRegistro;
            pagina->quantidadeElementos++;
            qsort(pagina->elementos,pagina->quantidadeElementos,sizeof(ArvoreB_Elemento),ArvoreB_Compara);
            fseek(arvore->f, posicaoPagina, SEEK_SET);
            fwrite(pagina, sizeof(ArvoreB_Pagina),1,arvore->f);
        }
        else
        {
            memcpy(overflow.chave, chave, TAM_CHAVE);
            overflow.posicaoRegistro = posicaoRegistro;
            overflow.paginaDireita = 0;
            resp = ArvoreB_Split(arvore, posicaoPagina, pagina, &overflow);
        }
    }
    else // Não é folha
    {
        posicaoPaginaFilho = pagina->paginaEsquerda;
        for(i=0; i<pagina->quantidadeElementos; i++)
        {
            if(strncmp(chave,pagina->elementos[i].chave,TAM_CHAVE)<0)
            {
                break;
            }
            posicaoPaginaFilho = pagina->elementos[i].paginaDireita;
        }
        elementoSplit = ArvoreB_Insere_Recursiva(arvore,posicaoPaginaFilho,chave,posicaoRegistro);
        if(elementoSplit)
        {
            if(pagina->quantidadeElementos < TAM_PAG)
            {
                pagina->elementos[pagina->quantidadeElementos] = *elementoSplit;
                pagina->quantidadeElementos++;
                qsort(pagina->elementos,pagina->quantidadeElementos,sizeof(ArvoreB_Elemento),ArvoreB_Compara);
                fseek(arvore->f,posicaoPagina,SEEK_SET);
                fwrite(pagina,sizeof(ArvoreB_Pagina),1,arvore->f);
                free(elementoSplit);
            }
            else
            {
                resp = ArvoreB_Split(arvore, posicaoPagina, pagina, elementoSplit);
                free(elementoSplit);
            }
        }
    }
    ArvoreB_desalocaPagina(pagina);
    return resp;
}

void ArvoreB_Insere(ArvoreB* arvore, char chave[TAM_CHAVE], long posicaoRegistro )
{
    ArvoreB_Pagina *novaRaiz;
    ArvoreB_Elemento *elementoSplit;
    elementoSplit = ArvoreB_Insere_Recursiva(arvore, arvore->cabecalho->raiz, chave, posicaoRegistro);
    if(elementoSplit)
    {
        novaRaiz = ArvoreB_alocaPagina();
        novaRaiz->quantidadeElementos = 1;
        novaRaiz->elementos[0] = *elementoSplit;
        novaRaiz->paginaEsquerda = arvore->cabecalho->raiz;
        fseek(arvore->f, 0, SEEK_END);
        arvore->cabecalho->raiz = ftell(arvore->f);
        fwrite(novaRaiz,sizeof(ArvoreB_Pagina),1,arvore->f);
        ArvoreB_escreveCabecalho(arvore);
        ArvoreB_desalocaPagina(novaRaiz);
        free(elementoSplit);
    }
}

//Esta funcao gera um arquivo texto com os registros.
void ArvoreB_PrintDebug(ArvoreB* arvore)
{
    //VARIÁVEIS
    long posicaoPagina;
    int i;
    FILE *saida = fopen("debug.txt", "w");
    ArvoreB_Pagina *pagina = ArvoreB_alocaPagina(); //SEPARA UM ESPAÇO PARA UMA PÁGINA

    //VAI PRA RAIZ
    fseek(arvore->f, sizeof(ArvoreB_Cabecalho), SEEK_SET);

    //PEGA A POSIÇÃO ATUAL
    posicaoPagina = ftell(arvore->f);
    while(fread(pagina,sizeof(ArvoreB_Pagina),1,arvore->f))
    {
        if(posicaoPagina == arvore->cabecalho->raiz)
        {
            //printf("* ");
            fprintf(saida, "* ");
        }
        else
        {
            //printf("  ");
            fprintf(saida, "  ");
        }
        //printf("%5ld => %5ld|",posicaoPagina, pagina->paginaEsquerda);
        fprintf(saida, "%10ld => %10ld|",posicaoPagina, pagina->paginaEsquerda);
        for(i=0; i < pagina->quantidadeElementos; i++)
        {

            //printf("(%.14s)|%5ld|",pagina->elementos[i].chave, pagina->elementos[i].paginaDireita);
            fprintf(saida, "(%.14s - pos %10ld)|%10ld|",pagina->elementos[i].chave, pagina->elementos[i].posicaoRegistro, pagina->elementos[i].paginaDireita);

        }
        //printf("\n");
        fprintf(saida, "\n");
        posicaoPagina = ftell(arvore->f);
    }
    fclose(saida);
}
//Nesta função a busca é feita página a página
long buscaPos(ArvoreB* arvore, char nis[14])
{
    //VARIÁVEIS
    int i;
    ArvoreB_Pagina *pagina = ArvoreB_alocaPagina(); //SEPARA UM ESPAÇO PARA UMA PÁGINA

    fseek(arvore->f, arvore->cabecalho->raiz, SEEK_SET); // vai para raiz
 
    while(fread(pagina, sizeof(ArvoreB_Pagina), 1, arvore->f))
    {
        for(i = 0; i < pagina->quantidadeElementos; i++)
        {
            //printf( "|%.14s| ",pagina->elementos[i].chave);


            if(strcmp(nis, pagina->elementos[i].chave) == 0) //compara nis digitado com os elementos da pag
            {
                return pagina->elementos[i].posicaoRegistro; //retorna a posicao
            }
            else if(strcmp(nis, pagina->elementos[i].chave) < 0) // ve se esta na pagina anterior
            {
                if(i == 0) //se esta no primeiro elemento
                {
                    if(pagina->paginaEsquerda != 0) //E tiver elemento a esquerda, entao vai para a pagina da esq
                    {
                        fseek(arvore->f, pagina->paginaEsquerda, SEEK_SET); //vai para a esquerda da página
                    }
                    else //se não tiver filho, elemento, então para ai
                    {
                        return -1;
                    }
                }
                else //nos elementos do meio
                {
                    if(pagina->elementos[i-1].paginaDireita != 0) //tiver filhos a direita no elemento anterior
                    {
                        fseek(arvore->f, pagina->elementos[i-1].paginaDireita, SEEK_SET); //vai para esquerda do elemento, que é a direita do anterior
                    }
                    else
                    {
                        return -1;
                    }
                }
                break;
            }
            else if(strcmp(nis, pagina->elementos[i].chave) > 0) //vai para direita 
            {
                if(i+1 == pagina->quantidadeElementos) //ve se é ultimo elemento
                {
                    if(pagina->elementos[i].paginaDireita != 0) //se tem elemento a direita
                    {
                        fseek(arvore->f, pagina->elementos[i].paginaDireita, SEEK_SET); //passa para prox pag à direita
                    }
                    else
                    {
                        return -1;
                    }
                    break;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    return -1;
}

//Aqui a busca é feita em todo o arquivo, e esta busca utiliza a função buscaPos, para que a pesquisa possa ser feita em cada página.
void buscaNoArquivo(ArvoreB* a, char bolsa[20])
{
    FILE *f = fopen(bolsa, "r");
    printf("\nDigite o NIS: ");
    char nis[14];
    scanf("%s",&nis); //Aqui é feita a leitura de cada "caracter" referente ao nis.

    long pos = buscaPos(a, nis); //"pos" vai receber o valor que a funcao "buscaPos" retorna, no caso, a posicao onde o nis se encontra.
    if(pos > -1) //Se o valor for uma posicao valida, existente, o seguinte bloco de codigo sera executado:
    {
        char linha[2048];
        fseek(f, pos, SEEK_SET); //posicionar a cabeca de leitura na posicao "pos"
        fgets(linha, 2048, f);   //ler um caracter de 2048 bytes no arquivo "f"
        printf("\n%s", linha);   //O registro e exibido.
    }
    else
    {
        printf("\nNao achou\n");
    }
    fclose(f);
}


typedef struct _Bolsa Bolsa;
struct _Bolsa
{
    char chave[16];
    long posicao;
};

void gerarArvore(ArvoreB* a, char bolsa[20])
{
    char linha[2048]; //por ter tamanhos variados chuta um valor grande
    Bolsa b; 
    char* campo;
    FILE *f = fopen(bolsa, "r");
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    rewind(f);

    b.posicao = ftell(f);
    fgets(linha, 2048, f);
    
    printf("\nInserindo na arvore...\n");
    printf("\n%ld / %ld bytes\t restam %ld bytes", b.posicao, tamanho, (tamanho-b.posicao));
    while(!feof(f))
    {
        campo = strtok(linha,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        strcpy(b.chave, campo);

        if(strlen(b.chave) != 14)
        {
            b.posicao = ftell(f);
            fgets(linha, 2048, f);
            continue;
        }
        
        //printf("nis => %s esta em %ld\n", b.chave, b.posicao);
        ArvoreB_Insere(a, b.chave, b.posicao);
        b.posicao = ftell(f);
 
        if(b.posicao % 20000 == 0)
            printf("\n%ld / %ld bytes\t restam %ld bytes", b.posicao, tamanho, (tamanho-b.posicao));
        fgets(linha, 2048, f);

    }
    printf("\n%d / %ld bytes\t restam 0 bytes", b.posicao, tamanho);
    printf("\n\nInserido\n");
    fclose(f);
}


void geraListaNis(char bolsa[20])
{
    printf("\nDigite a quantidade de linhas para adicionar a lista: ");
    long limite;
    scanf("%ld", &limite);
    srand((unsigned)time(NULL));
    char linha[2048]; //por ter tamanhos variados chuta um valor grande
    Bolsa b; 
    char* campo;
    FILE *f = fopen(bolsa, "r");
    FILE *f2 = fopen("nis.dat", "w");
    if(bolsa == "bolsa.csv")
        fgets(linha, 2048, f); //descarta a primeira linha do arquivo [IMPORTANTE NO BOLSA.CSV]
    fgets(linha, 2048, f);
    printf("\nGerando lista...\n\n");
    long x = 0;
    //Neste "while" vamos avançando coluna por coluna, a cada linha. Exceto a primeira linha, que deve ser ignorada por conter apenas o nome de cada campo.
    while(!feof(f) && (x++ < limite))
    {
        campo = strtok(linha,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        campo = strtok(NULL,"\t");
        strcpy(b.chave, campo);

        fprintf(f2, "%s\n", b.chave);

        fgets(linha, 2048, f);
    }
    printf("\nCompleto.\n");
    fclose(f);
    fclose(f2);
}

//A função a seguir serve para calcular o tempo gasto em cada busca.
void buscaParaGrafico(ArvoreB* a)
{
    clock_t c2, c1; //variáveis de clock do processador.
    float tempo; 
    char linha[2048];
    FILE *f = fopen("nis.dat", "r");
    fgets(linha, 2048, f);
    char nis[14];
    sprintf(nis, "%.14s", linha);
    printf("\nQual eh o numero maximo de elementos a serem buscados? ");
    int limite;
    scanf("%d", &limite);
    int x = 0;
    c1 = clock(); //Calcular o tempo decorrido desde 1o de janeiro de 1970
    while(!feof(f) && (x++ < limite))
    {
        sprintf(nis, "%.14s", linha);
        //printf("%ld\n", buscaPos(a, nis));
        buscaPos(a, nis); //chama a buscaPos, para pesquisar página a página.
        fgets(linha, 2048, f);
    }
    c2 = clock(); //Calcular o tempo decorrido desde 1o de janeiro de 1970
    fclose(f);
    tempo = (c2 - c1)*1000/CLOCKS_PER_SEC; //Calcular a diferenca entre os tempos decorridos para obtermos o tempo gasto para a busca.
    printf("\n\n%f milissegundos", tempo);
}

void trocaNome(char *bolsa)
{
    printf("\ncom qual arquivo de bolsa deseja trabalhar? ");
    scanf("%s", bolsa);
}


void main(int argc, char **argv){
     setlocale(LC_ALL, "Portuguese");
    
    ArvoreB* a = ArvoreB_Abre("arvore.dat"); //Criar o arquivo "arvore.dat"
    
    char bolsa[20] = "bolsa.csv"; //passar o nome do arquivo "bolsa" para a variavel "bolsa[20]" para ser pesquisado nas funcoes.

       /*
        printf("\n        INSTRUCOES         \n");
        printf("O menu fornece uma serie de funcoes que deve ser executada numa ordem correta para que possamos fazer a pesquisa com sucesso. \n");
        printf("Primeiro, geramos uma amostra com registros aleatorios para a busca ser mais pratica. Isso foi utilizado para que eu pudesse verificar a presenca dos registros la.");
        printf("Depois, chamamos a funcao 'trocaNome', para mudar o nome do arquivo que contem as amostras a cada execucao.");
        printf("Em seguida, podemos indexar esses registros na estrutura. Sera exibida na tela a indexacao ocorrendo. ");
        printf("Depois, geramos uma lista com o NIS, para calcularmos o tempo gasto com a busca em X registros, e um arquivo de 'debug' para visualizarmos algum nis que a gente queira pesquisar pra testar a presenca dele na arvore. ");
        printf("Por ultimo, fazemos as buscas. Uma funcao com a busca em qualquer um dos arquivos definidos no inicio, seja no arquivo original do bolsa ou numa amostra. Em seguida calculamos o tempo gasto em funcao do numero de resgistros. ");
        printf("Os arquivos de teste nos garantem que estejamos visualizando dados validos. \n\n");
       */

       printf("     INSTRUCOES    ");
       printf("O menu oferece diversas funcoes que devem ser executadas na ordem em que estao para que a busca seja bem sucedida. ");
       printf("\n\n1 - Gerar amostra : Esta funcao ira gerar uma lista\n com registros aleatorios, para a arvore ser feita com base nessa amostra. ");
       printf("\n\n2 - Trocar Nome : Esta funcao serve para mudar o arquivo no qual a pesquisa esta sendo feita.\nPois num momento, podemos pesquisar no arquivo original, e em outro, numa amostra.(Opcional). \n ");
       printf("\n\nA opcao 3 gera a arvore a partir do arquivo escolhido como referencia.  ");
       printf("\n\nA opcao 4 gera ma lista de NIS para auxiliar no calculo do tempo de busca. e a opcao 5 gera um 'debug', com os registros na arvore. \nEsses arquivos sao para testes e servem para verificarmos a presenca de um certo NIS no arquivo.");
       printf("\n\nNa opcao 6, finalmente e feita a busca por NIS. \nO usuario pesquisa com o auxilio dos arquivos dos passos 4 e 5. ");
       printf("\n\nNa opcao 7, e calculado o tempo de busca em funcao do numero de registros a serem pesquisados.\nNao e obrigatorio percorrer todo o arquivo,\npor isso o usuario pode informar uma quantidade menor ou igual ao tamannho do arquivo. \n");
    int op;
    do
    {
        printf("\nUtilizando '%s'\n", bolsa);
        printf("E importante que as instrucoes sejam lidas com atencao, para o correto funcionamento do programa. ");
        printf("\nSELECIONE UMA OPCAO\n\n");
        printf("\n1 - Trocar  nome do arquivo de referencia \n2 - Gerar uma amostra com registros aleatorios.   \n3 - Gerar Arvore(arvore e gerada a partir da referencia escolhida).  \n4 - Gerar lista de NIS.  \n5 - gerar debug  \n6 - Buscar no arquivo \n7 - Busca para calcular tempo \n0 - sair\n\n");
        scanf("%d", &op);
        
        switch(op)
        {
            /*
            case 1: buscaParaGrafico(a); break;
            case 2: buscaNoArquivo(a, bolsa); break;
            case 3: geraListaNis(bolsa); break;
            case 4: ArvoreB_PrintDebug(a); printf("\nDebug gerado\n"); break;
            case 5: gerarArvore(a, bolsa); break;
            */

            case 3: gerarArvore(a, bolsa); break; //gerarArvore quer dizer que o arquivo do bolsa será lido para pos seus registros serem inseridos na arvore.
            case 4: geraListaNis(bolsa); break;
            case 5: ArvoreB_PrintDebug(a); printf("\nDebug gerado\n"); break; //O debug serve para visualizarmos registros, e usa-los para testar na pesquisa na arvore com mas amostras.
            case 6: buscaNoArquivo(a, bolsa); break;
            case 7: buscaParaGrafico(a); break;
            case 2: aleatorios(bolsa);break; //Gerar uma amostra para ser feita pesquisa 
            case 1: trocaNome(bolsa); break;
            
        }

        printf("\n");

    }
    while(op != 0);

    ArvoreB_Fecha(a);
   
}