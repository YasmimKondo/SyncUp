/*
 * SYNC UP - rede social inclusiva (versão terminal, só C)
 *
 * Como compilar:  gcc syncup.c -o syncup
 * Como rodar:     ./syncup        (no Windows: syncup.exe)
 *
 * Organização do arquivo (de cima para baixo):
 *   1. Includes e constantes
 *   2. Structs (os "moldes" dos dados)
 *   3. Dados do programa (vetores globais)
 *   4. Utilitários (ler teclado, limpar tela, pausar)
 *   5. Acessibilidade (fonte ampliada, voz, descrição de imagem)
 *   6. Arquivos (salvar e carregar)
 *   7. Usuários (criar conta e login)
 *   8. Posts e feed
 *   9. main (menu inicial)
 *
 * Obs.: os textos que aparecem na tela estão sem acento de propósito,
 * para não dar problema de codificação em terminais diferentes.
 */

/* ============================================================
 * 1. INCLUDES E CONSTANTES
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>   // system(), atoi()
#include <string.h>   // strcpy, strcmp, strlen, strchr, strstr
#include <ctype.h>    // toupper()
#include <time.h>     // time_t, time(), localtime(), strftime()

#ifdef _WIN32
#include <windows.h>  // Sleep() no Windows
#else
#include <unistd.h>   // usleep() no Linux/Mac
#endif

// #define cria um nome para um valor fixo. Se quiser mudar o limite,
// muda aqui e o programa inteiro se ajusta.
#define MAX_USUARIOS    50
#define MAX_POSTS       100
#define MAX_COMENTARIOS 10

#define ARQ_USUARIOS "usuarios.dat"
#define ARQ_POSTS    "posts.dat"

// NOVO: códigos ANSI. Quando impressos, o terminal muda a cor do texto.
// COR_RESET volta ao normal.
#define COR_RESET  "\033[0m"
#define COR_TITULO "\033[1;36m"   // ciano em negrito
#define COR_OK     "\033[1;32m"   // verde em negrito
#define COR_ERRO   "\033[1;31m"   // vermelho em negrito

/* ============================================================
 * 2. STRUCTS
 * ============================================================ */

// Um comentário: quem escreveu e o texto.
typedef struct {
    char autor[50];
    char texto[100];
} Comentario;

// Um usuário. fonteGrande guarda a preferência de acessibilidade
// (1 = ligada, 0 = desligada) e fica salva junto com a conta.
typedef struct {
    int  id;
    char nome[50];
    char senha[20];
    int  fonteGrande;
} Usuario;

// Uma publicação. Repare que ela contém um VETOR de Comentario
// (struct dentro de struct, como vimos na aula 2).
// Guardamos só o NOME do autor (e não o Usuario inteiro) para
// manter o projeto simples e evitar repetir dados.
typedef struct {
    int        id;
    char       autor[50];
    char       texto[200];
    char       imagem[60];            // nome do arquivo, ex: "praia.jpg" (pode ser vazio)
    char       descricaoImagem[150];  // texto gerado pela "IA" simulada
    int        curtidas;
    time_t     horario;               // data e hora da publicação
    int        totalComentarios;
    Comentario comentarios[MAX_COMENTARIOS];
} Publicacao;

/* ============================================================
 * 3. DADOS DO PROGRAMA
 * Vetores globais + um contador de quantos já estão em uso.
 * ============================================================ */
Usuario    usuarios[MAX_USUARIOS];
int        totalUsuarios = 0;

Publicacao posts[MAX_POSTS];
int        totalPosts = 0;

/* ============================================================
 * 4. UTILITÁRIOS
 * ============================================================ */

// NOVO: lê uma linha inteira do teclado (aceita espaços) de forma segura.
// fgets não deixa estourar o tamanho do vetor. Ele guarda também o "Enter"
// (\n), então tiramos esse \n no final.
// Se a pessoa digitar mais do que cabe, descartamos o excesso para ele
// não "vazar" para a próxima leitura.
void lerTexto(char *destino, int tamanho) {
    if (fgets(destino, tamanho, stdin) == NULL) {  // acabou a entrada
        destino[0] = '\0';
        return;
    }
    if (strchr(destino, '\n') == NULL) {           // não achou \n = digitou demais
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
    destino[strcspn(destino, "\n")] = '\0';        // troca o \n por fim de texto
}

// NOVO: lê um número lendo como texto e convertendo com atoi.
// É mais seguro que scanf("%d"), que costuma deixar lixo no buffer.
// Se a pessoa digitar letras, atoi devolve 0.
int lerInteiro() {
    char buffer[20];
    lerTexto(buffer, sizeof(buffer));
    return atoi(buffer);
}

// Espera a pessoa apertar Enter (para dar tempo de ler a tela).
void pressionarEnter() {
    char lixo[5];
    printf("\nPressione Enter para continuar...");
    lerTexto(lixo, sizeof(lixo));
}

// Limpa o terminal. system() executa um comando do sistema operacional.
void limparTela() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Pausa o programa por alguns milissegundos (usado na "voz" e na "IA").
void pausar(int milissegundos) {
#ifdef _WIN32
    Sleep(milissegundos);
#else
    usleep(milissegundos * 1000);
#endif
}

/* ============================================================
 * 5. ACESSIBILIDADE
 * ============================================================ */

// RECURSO 1: fonte ampliada.
// O terminal não muda o tamanho da letra, então SIMULAMOS:
// maiúsculas + espaço entre as letras + espaço maior entre palavras.
void exibirTexto(const char *texto, int ampliado) {
    if (!ampliado) {
        printf("%s\n", texto);
        return;
    }
    printf("\n");
    for (int i = 0; texto[i] != '\0'; i++) {
        if (texto[i] == ' ') {
            printf("   ");
        } else {
            printf("%c ", toupper((unsigned char)texto[i]));
        }
    }
    printf("\n");
}

// RECURSO 2: leitura em voz alta (simulada).
// Imprime o texto aos poucos, fazendo uma pausa a cada palavra.
// fflush(stdout) força o texto a aparecer na hora, sem esperar o fim da linha.
void lerEmVozAlta(const char *texto) {
    printf("\n[VOZ] Lendo em voz alta...\n> ");
    for (int i = 0; texto[i] != '\0'; i++) {
        putchar(texto[i]);
        fflush(stdout);
        if (texto[i] == ' ') pausar(250);
    }
    printf("\n");
}

// RECURSO 3: descrição de imagem por "IA" (simulada).
// Olha o nome do arquivo e devolve uma descrição pronta.
// strstr procura um pedaço de texto dentro de outro texto.
// Em um app real, aqui entraria uma API de visão computacional.
void gerarDescricaoIA(const char *arquivo, char *destino) {
    if (strstr(arquivo, "praia")) {
        strcpy(destino, "Uma praia ensolarada com mar azul e areia clara.");
    } else if (strstr(arquivo, "gato")) {
        strcpy(destino, "Um gato deitado em um sofa, olhando para a camera.");
    } else if (strstr(arquivo, "festa")) {
        strcpy(destino, "Um grupo de pessoas sorrindo em uma festa com baloes coloridos.");
    } else {
        strcpy(destino, "Imagem sem descricao reconhecida (uma IA real analisaria a imagem aqui).");
    }
}

/* ============================================================
 * 6. ARQUIVOS (salvar e carregar)
 * Gravamos o vetor INTEIRO de structs em arquivo binário.
 *   "wb" = escrever binário    "rb" = ler binário
 * fwrite/fread copiam bytes direto da memória, sem converter para texto.
 * ============================================================ */
void salvarDados() {
    FILE *f = fopen(ARQ_USUARIOS, "wb");
    if (f != NULL) {
        fwrite(&totalUsuarios, sizeof(int), 1, f);                 // 1º: quantos são
        fwrite(usuarios, sizeof(Usuario), totalUsuarios, f);       // 2º: os dados
        fclose(f);
    }

    f = fopen(ARQ_POSTS, "wb");
    if (f != NULL) {
        fwrite(&totalPosts, sizeof(int), 1, f);
        fwrite(posts, sizeof(Publicacao), totalPosts, f);
        fclose(f);
    }
}

void carregarDados() {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    if (f != NULL) {   // na primeira execução o arquivo não existe, e tudo bem
        if (fread(&totalUsuarios, sizeof(int), 1, f) == 1 &&
            totalUsuarios >= 0 && totalUsuarios <= MAX_USUARIOS) {
            if (fread(usuarios, sizeof(Usuario), totalUsuarios, f) != (size_t)totalUsuarios) {
                totalUsuarios = 0;
            }
        } else {
            totalUsuarios = 0;
        }
        fclose(f);
    }

    f = fopen(ARQ_POSTS, "rb");
    if (f != NULL) {
        if (fread(&totalPosts, sizeof(int), 1, f) == 1 &&
            totalPosts >= 0 && totalPosts <= MAX_POSTS) {
            if (fread(posts, sizeof(Publicacao), totalPosts, f) != (size_t)totalPosts) {
                totalPosts = 0;
            }
        } else {
            totalPosts = 0;
        }
        fclose(f);
    }
}

/* ============================================================
 * 7. USUÁRIOS
 * ============================================================ */

// Devolve a posição do usuário no vetor, ou -1 se não achar.
int buscarUsuario(const char *nome) {
    for (int i = 0; i < totalUsuarios; i++) {
        // strcmp devolve 0 quando os textos são iguais
        if (strcmp(usuarios[i].nome, nome) == 0) return i;
    }
    return -1;
}

void criarConta() {
    if (totalUsuarios >= MAX_USUARIOS) {
        printf("%sLimite de usuarios atingido.%s\n", COR_ERRO, COR_RESET);
        return;
    }

    char nome[50], senha[20];

    printf("\nNome de usuario: ");
    lerTexto(nome, sizeof(nome));
    if (strlen(nome) == 0) {
        printf("%sO nome nao pode ser vazio.%s\n", COR_ERRO, COR_RESET);
        return;
    }
    if (buscarUsuario(nome) != -1) {
        printf("%sJa existe um usuario com esse nome.%s\n", COR_ERRO, COR_RESET);
        return;
    }

    printf("Senha: ");
    lerTexto(senha, sizeof(senha));

    // Preenche a próxima ficha livre do vetor.
    Usuario *novo = &usuarios[totalUsuarios];   // endereço da ficha (veja nota sobre ponteiros abaixo)
    novo->id = totalUsuarios + 1;
    strcpy(novo->nome, nome);
    strcpy(novo->senha, senha);
    novo->fonteGrande = 0;
    totalUsuarios++;

    salvarDados();
    printf("%sConta criada com sucesso!%s\n", COR_OK, COR_RESET);
}

// Devolve a posição do usuário logado, ou -1 se usuário/senha estiverem errados.
int fazerLogin() {
    char nome[50], senha[20];

    printf("\nNome de usuario: ");
    lerTexto(nome, sizeof(nome));
    printf("Senha: ");
    lerTexto(senha, sizeof(senha));

    int pos = buscarUsuario(nome);
    if (pos != -1 && strcmp(usuarios[pos].senha, senha) == 0) {
        return pos;
    }
    return -1;
}

/* ============================================================
 * 8. POSTS E FEED
 *
 * NOTA SOBRE PONTEIROS (novidade!):
 * Quando passamos uma struct para uma função "normal", a função recebe
 * uma CÓPIA, e mudar a cópia não altera a original. Para alterar a
 * original, passamos o ENDEREÇO dela (&posts[atual]) e a função recebe
 * um PONTEIRO (Publicacao *p).
 * Para acessar um campo através de um ponteiro, usamos a seta:
 *     p->curtidas     (equivale a (*p).curtidas)
 * ============================================================ */

// Cria um post novo no final do vetor. Devolve 1 se deu certo, 0 se não.
// "const char *" significa: recebo um texto que a função não vai alterar.
int adicionarPost(const char *autor, const char *texto, const char *imagem) {
    if (totalPosts >= MAX_POSTS) return 0;

    Publicacao *p = &posts[totalPosts];
    p->id = totalPosts + 1;
    strcpy(p->autor, autor);
    strcpy(p->texto, texto);
    strcpy(p->imagem, imagem);

    if (strlen(imagem) > 0) {
        gerarDescricaoIA(imagem, p->descricaoImagem);
    } else {
        p->descricaoImagem[0] = '\0';   // texto vazio
    }

    p->curtidas = 0;
    p->horario = time(NULL);   // time(NULL) = agora
    p->totalComentarios = 0;

    totalPosts++;
    return 1;
}

// Posts de exemplo, para o feed não começar vazio.
void criarPostsExemplo() {
    adicionarPost("Sync Up", "Bem-vindos ao Sync Up! Aqui todo mundo tem voz.", "");
    adicionarPost("Ana", "Fui na praia hoje, o dia estava lindo.", "praia.jpg");
    adicionarPost("Carlos", "Alguem recomenda apps de leitura de tela?", "");
}

// Pergunta o texto (e a imagem) e publica.
int criarPost(Usuario *u) {
    if (totalPosts >= MAX_POSTS) return 0;

    char texto[200], imagem[60];

    printf("\nTexto do post (Enter vazio cancela): ");
    lerTexto(texto, sizeof(texto));
    if (strlen(texto) == 0) return 0;

    printf("Nome da imagem, ex: gato.jpg (Enter para nenhuma): ");
    lerTexto(imagem, sizeof(imagem));

    return adicionarPost(u->nome, texto, imagem);
}

// Adiciona um comentário ao post. Devolve a mensagem que o feed vai mostrar.
// (Textos entre aspas no código ficam guardados no programa o tempo todo,
//  por isso é seguro devolver "const char *" deles.)
const char *comentar(Publicacao *p, Usuario *u) {
    if (p->totalComentarios >= MAX_COMENTARIOS) {
        return "Limite de comentarios deste post atingido.";
    }

    char texto[100];
    printf("\nSeu comentario (Enter vazio cancela): ");
    lerTexto(texto, sizeof(texto));
    if (strlen(texto) == 0) return "Comentario cancelado.";

    Comentario *c = &p->comentarios[p->totalComentarios];
    strcpy(c->autor, u->nome);
    strcpy(c->texto, texto);
    p->totalComentarios++;

    return "Comentario publicado!";
}

void descreverImagem(Publicacao *p) {
    if (strlen(p->imagem) == 0) {
        printf("\nEste post nao tem imagem.\n");
        return;
    }
    printf("\n[IA] Analisando a imagem '%s'...\n", p->imagem);
    pausar(1200);   // só para dar a sensação de que a IA está "pensando"
    printf("[IA] Descricao: %s\n", p->descricaoImagem);
}

// Mostra UM post na tela, com data, texto, curtidas e comentários.
void exibirPost(Publicacao *p, int posicao, int ampliado) {
    // NOVO: transforma o time_t em texto legível (dd/mm/aaaa hh:mm)
    char dataTexto[20];
    struct tm *t = localtime(&p->horario);
    strftime(dataTexto, sizeof(dataTexto), "%d/%m/%Y %H:%M", t);

    printf("%s=== Post %d de %d ===%s\n", COR_TITULO, posicao + 1, totalPosts, COR_RESET);
    printf("Autor: %s  |  %s\n\n", p->autor, dataTexto);

    exibirTexto(p->texto, ampliado);

    if (strlen(p->imagem) > 0) {
        printf("\n[Imagem anexada: %s]\n", p->imagem);
    }

    printf("\nCurtidas: %d\n", p->curtidas);
    printf("Comentarios (%d):\n", p->totalComentarios);
    for (int i = 0; i < p->totalComentarios; i++) {
        printf("  - %s: %s\n", p->comentarios[i].autor, p->comentarios[i].texto);
    }
}

// O FEED: mostra um post por vez e deixa o usuário interagir.
// "atual" guarda qual post está na tela.
void feed(Usuario *usuario) {
    int atual = 0;
    int opcao;
    const char *mensagem = "";   // recado que aparece abaixo do post (ex: "Voce curtiu!")

    do {
        limparTela();
        exibirPost(&posts[atual], atual, usuario->fonteGrande);

        if (strlen(mensagem) > 0) {
            printf("\n%s>> %s%s\n", COR_OK, mensagem, COR_RESET);
            mensagem = "";
        }

        printf("\n--------------------------------------------\n");
        printf("1-Curtir   2-Comentar   3-Proximo   4-Anterior\n");
        printf("5-Ouvir post   6-Descrever imagem (IA)\n");
        printf("7-Fonte ampliada (liga/desliga)   8-Novo post\n");
        printf("0-Sair da conta\n> ");
        opcao = lerInteiro();

        switch (opcao) {
            case 1:
                posts[atual].curtidas++;
                mensagem = "Voce curtiu este post!";
                break;
            case 2:
                mensagem = comentar(&posts[atual], usuario);
                break;
            case 3:
                // O % (resto da divisão) faz o feed "dar a volta":
                // depois do último post, volta para o primeiro.
                atual = (atual + 1) % totalPosts;
                break;
            case 4:
                atual = (atual - 1 + totalPosts) % totalPosts;
                break;
            case 5:
                lerEmVozAlta(posts[atual].texto);
                pressionarEnter();
                break;
            case 6:
                descreverImagem(&posts[atual]);
                pressionarEnter();
                break;
            case 7:
                usuario->fonteGrande = !usuario->fonteGrande;   // inverte 0/1
                mensagem = usuario->fonteGrande ? "Fonte ampliada LIGADA."
                                                : "Fonte ampliada DESLIGADA.";
                break;
            case 8:
                if (criarPost(usuario)) {
                    atual = totalPosts - 1;   // mostra o post recém-criado
                    mensagem = "Post publicado!";
                } else {
                    mensagem = "Post nao publicado.";
                }
                break;
            case 0:
                break;   // o while abaixo encerra o laço
            default:
                mensagem = "Opcao invalida.";
        }
    } while (opcao != 0);

    salvarDados();   // guarda curtidas, comentários, posts e preferências
}

/* ============================================================
 * 9. MAIN: menu inicial
 * ============================================================ */
int main() {
#ifdef _WIN32
    system("");   // truque para o terminal do Windows 10+ entender as cores ANSI
#endif

    carregarDados();                       // lê os arquivos (se existirem)
    if (totalPosts == 0) criarPostsExemplo();

    int opcao;
    do {
        limparTela();
        printf("%s", COR_TITULO);
        printf("  ____                       _   _       \n");
        printf(" / ___| _   _ _ __   ___    | | | |_ __  \n");
        printf(" \\___ \\| | | | '_ \\ / __|   | | | | '_ \\ \n");
        printf("  ___) | |_| | | | | (__    | |_| | |_) |\n");
        printf(" |____/ \\__, |_| |_|\\___|    \\___/| .__/ \n");
        printf("        |___/                      |_|    \n");
        printf("%s", COR_RESET);
        printf("   A rede social para todo mundo\n\n");
        printf("1 - Login\n");
        printf("2 - Criar conta\n");
        printf("0 - Sair\n> ");
        opcao = lerInteiro();

        switch (opcao) {
            case 1: {
                int logado = fazerLogin();
                if (logado >= 0) {
                    feed(&usuarios[logado]);   // passa o ENDEREÇO do usuário logado
                } else {
                    printf("%sUsuario ou senha incorretos.%s\n", COR_ERRO, COR_RESET);
                    pressionarEnter();
                }
                break;
            }
            case 2:
                criarConta();
                pressionarEnter();
                break;
            case 0:
                printf("\nAte logo!\n");
                break;
            default:
                printf("%sOpcao invalida.%s\n", COR_ERRO, COR_RESET);
                pressionarEnter();
        }
    } while (opcao != 0);

    salvarDados();
    return 0;
}