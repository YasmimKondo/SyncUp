/*
 * SYNC UP - rede social inclusiva (versão terminal, só C)
 * VERSÃO COM 5 "TABELAS" (structs) E CRUD COMPLETO
 *
 * Como compilar:  gcc syncup.c -o syncup
 * Como rodar:     ./syncup        (no Windows: .\syncup.exe)
 *
 * As 5 "tabelas" (cada uma é uma struct):
 *   1. Usuario      - quem usa a rede
 *   2. Publicacao   - os posts
 *   3. Comentario   - comentários (ficam dentro de cada post)
 *   4. Curtida      - quem curtiu qual post        (NOVA)
 *   5. Seguidor     - quem segue quem              (NOVA)
 *
 * O CRUD (Create, Read, Update, Delete) no programa:
 *   CREATE: criar conta, criar post, comentar, curtir, seguir
 *   READ:   login, feed (um post por vez), perfil
 *   UPDATE: editar conta (nome, e-mail e senha) - NOVO
 *   DELETE: excluir post, descurtir, deixar de seguir - NOVO
 *
 * Organização do arquivo (de cima para baixo):
 *   1. Includes e constantes
 *   2. Structs (as tabelas)
 *   3. Dados do programa (vetores globais)
 *   4. Utilitários (ler teclado, limpar tela, pausar)
 *   5. Acessibilidade (fonte ampliada, voz, descrição de imagem)
 *   6. Arquivos (salvar e carregar)
 *   7. Usuários (criar conta, login, editar conta)
 *   8. Curtidas e seguidores
 *   9. Posts, perfil e feed
 *  10. main (menu inicial)
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
#define MAX_CURTIDAS    500
#define MAX_SEGUIDORES  200

// NOVO: agora TODOS os dados ficam em um arquivo só.
#define ARQ_DADOS "syncup.dat"

// Códigos ANSI. Quando impressos, o terminal muda a cor do texto.
// COR_RESET volta ao normal.
#define COR_RESET  "\033[0m"
#define COR_TITULO "\033[1;36m"   // ciano em negrito
#define COR_OK     "\033[1;32m"   // verde em negrito
#define COR_ERRO   "\033[1;31m"   // vermelho em negrito

/* ============================================================
 * 2. STRUCTS (as 5 tabelas)
 * Em cada uma, o CREATE TABLE equivalente em SQL, só como comentário.
 * ============================================================ */

// TABELA 3: um comentário. Fica dentro da Publicacao (vetor).
// SQL: "CREATE TABLE comentarios (id INTEGER PRIMARY KEY, publicacao_id INTEGER,
//                                 autor TEXT, texto TEXT);"
typedef struct {
    char autor[50];
    char texto[100];
} Comentario;

// TABELA 1: um usuário. fonteGrande guarda a preferência de acessibilidade
// (1 = ligada, 0 = desligada) e fica salva junto com a conta.
// SQL: "CREATE TABLE usuarios (id INTEGER PRIMARY KEY, nome TEXT, email TEXT,
//                             senha TEXT, fonte_grande INTEGER);"
typedef struct {
    int  id;
    char nome[50];
    char email[60];
    char senha[20];
    int  fonteGrande;
} Usuario;

// TABELA 2: uma publicação. Contém um VETOR de Comentario
// (struct dentro de struct). Guardamos o NOME do autor para manter
// o projeto simples.
// Repare: não existe mais o campo "curtidas". Agora as curtidas ficam na
// tabela Curtida, e o total é CONTADO quando precisamos.
// SQL: "CREATE TABLE publicacoes (id INTEGER PRIMARY KEY, autor TEXT, texto TEXT,
//                                 imagem TEXT, descricao_imagem TEXT, horario TEXT);"
typedef struct {
    int        id;
    char       autor[50];
    char       texto[200];
    char       imagem[60];            // nome do arquivo, ex: "praia.jpg" (pode ser vazio)
    char       descricaoImagem[150];  // texto gerado pela "IA" simulada
    time_t     horario;               // data e hora da publicação
    int        totalComentarios;
    Comentario comentarios[MAX_COMENTARIOS];
} Publicacao;

// TABELA 4 (NOVA): uma curtida = "este usuário curtiu este post".
// É uma tabela de RELACIONAMENTO: liga um usuário a uma publicação.
// Uma pessoa só pode curtir o mesmo post uma vez.
// SQL: "CREATE TABLE curtidas (usuario_id INTEGER, publicacao_id INTEGER);"
typedef struct {
    int usuarioId;
    int publicacaoId;
} Curtida;

// TABELA 5 (NOVA): "seguidor segue seguido". Também é de relacionamento:
// liga um usuário a outro usuário.
// SQL: "CREATE TABLE seguidores (seguidor_id INTEGER, seguido_id INTEGER);"
typedef struct {
    int seguidorId;
    int seguidoId;
} Seguidor;

/* ============================================================
 * 3. DADOS DO PROGRAMA
 * Vetores globais + um contador de quantos já estão em uso.
 * ============================================================ */
Usuario    usuarios[MAX_USUARIOS];
int        totalUsuarios = 0;

Publicacao posts[MAX_POSTS];
int        totalPosts = 0;

Curtida    curtidas[MAX_CURTIDAS];
int        totalCurtidas = 0;

Seguidor   seguidores[MAX_SEGUIDORES];
int        totalSeguidores = 0;

// NOVO: com a exclusão de posts, usar "totalPosts + 1" como id poderia
// repetir um id. Por isso guardamos o próximo id livre.
int        proximoIdPost = 1;

/* ============================================================
 * 4. UTILITÁRIOS
 * ============================================================ */

// Lê uma linha inteira do teclado (aceita espaços) de forma segura.
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

// Lê um número lendo como texto e convertendo com atoi.
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
 * Gravamos cada vetor INTEIRO de structs em um arquivo binário só.
 *   "wb" = escrever binário    "rb" = ler binário
 * fwrite/fread copiam bytes direto da memória, sem converter para texto.
 *
 * NOVO: como temos 4 vetores para salvar, criamos duas funções
 * ajudantes para não repetir código. Elas recebem "void *", um
 * ponteiro genérico que aceita um vetor de QUALQUER tipo.
 * "size_t" é o tipo usado para tamanhos (o que o sizeof devolve).
 *
 * ATENÇÃO: se você mudar uma struct, apague o arquivo syncup.dat,
 * porque o tamanho de cada ficha mudou e o arquivo antigo não serve mais.
 * ============================================================ */

// Grava um bloco: primeiro QUANTOS itens, depois os itens.
void gravarBloco(FILE *f, const void *vetor, size_t tamanhoItem, int total) {
    fwrite(&total, sizeof(int), 1, f);
    fwrite(vetor, tamanhoItem, total, f);
}

// Lê um bloco. "int *total" é um ponteiro para o contador, assim a função
// consegue preencher o contador original. Devolve 1 se deu certo, 0 se não.
int lerBloco(FILE *f, void *vetor, size_t tamanhoItem, int *total, int maximo) {
    if (fread(total, sizeof(int), 1, f) != 1) return 0;
    if (*total < 0 || *total > maximo) return 0;   // valor estranho = arquivo ruim
    return fread(vetor, tamanhoItem, *total, f) == (size_t)*total;
}

void salvarDados() {
    FILE *f = fopen(ARQ_DADOS, "wb");
    if (f == NULL) return;
    gravarBloco(f, usuarios,   sizeof(Usuario),    totalUsuarios);
    gravarBloco(f, posts,      sizeof(Publicacao), totalPosts);
    gravarBloco(f, curtidas,   sizeof(Curtida),    totalCurtidas);
    gravarBloco(f, seguidores, sizeof(Seguidor),   totalSeguidores);
    fclose(f);
}

void carregarDados() {
    FILE *f = fopen(ARQ_DADOS, "rb");
    if (f == NULL) return;   // na primeira execução o arquivo não existe, e tudo bem

    // O && para no primeiro bloco que falhar.
    int ok = lerBloco(f, usuarios,   sizeof(Usuario),    &totalUsuarios,   MAX_USUARIOS)
          && lerBloco(f, posts,      sizeof(Publicacao), &totalPosts,      MAX_POSTS)
          && lerBloco(f, curtidas,   sizeof(Curtida),    &totalCurtidas,   MAX_CURTIDAS)
          && lerBloco(f, seguidores, sizeof(Seguidor),   &totalSeguidores, MAX_SEGUIDORES);
    fclose(f);

    if (!ok) {   // arquivo corrompido: começa do zero
        totalUsuarios = 0;
        totalPosts = 0;
        totalCurtidas = 0;
        totalSeguidores = 0;
    }

    // Descobre o próximo id livre de post (o maior id existente + 1).
    proximoIdPost = 1;
    for (int i = 0; i < totalPosts; i++) {
        if (posts[i].id >= proximoIdPost) proximoIdPost = posts[i].id + 1;
    }
}

/* ============================================================
 * 7. USUÁRIOS (criar conta, login, editar conta)
 * ============================================================ */

// Procura um usuário pelo nome. Devolve a posição, ou -1 se não achar.
// strcmp devolve 0 quando os textos são iguais (== não funciona com texto).
int buscarUsuario(const char *nome) {
    for (int i = 0; i < totalUsuarios; i++) {
        if (strcmp(usuarios[i].nome, nome) == 0) return i;
    }
    return -1;
}

// Coloca um usuário novo no fim do vetor. Devolve 1 se deu certo.
int adicionarUsuario(const char *nome, const char *email, const char *senha) {
    if (totalUsuarios >= MAX_USUARIOS) return 0;

    // Ponteiro para a próxima ficha livre; a seta (->) acessa os campos.
    Usuario *novo = &usuarios[totalUsuarios];
    novo->id = totalUsuarios + 1;
    strcpy(novo->nome, nome);     // texto não aceita "=", por isso strcpy
    strcpy(novo->email, email);
    strcpy(novo->senha, senha);
    novo->fonteGrande = 0;

    totalUsuarios++;
    return 1;
}

// Usuários de exemplo (senha 123), para dar para testar curtir/seguir
// logo de cara. Os autores dos posts de exemplo existem de verdade.
void criarUsuariosExemplo() {
    adicionarUsuario("Sync Up", "contato@syncup.com", "123");
    adicionarUsuario("Ana", "ana@exemplo.com", "123");
    adicionarUsuario("Carlos", "carlos@exemplo.com", "123");
}

// CREATE: criar conta
void criarConta() {
    if (totalUsuarios >= MAX_USUARIOS) {
        printf("%sLimite de usuarios atingido.%s\n", COR_ERRO, COR_RESET);
        return;
    }

    char nome[50], email[60], senha[20];

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

    printf("E-mail: ");
    lerTexto(email, sizeof(email));
    printf("Senha: ");
    lerTexto(senha, sizeof(senha));

    adicionarUsuario(nome, email, senha);
    salvarDados();
    printf("%sConta criada com sucesso!%s\n", COR_OK, COR_RESET);
}

// READ: login. Devolve a posição do usuário logado, ou -1 se nome/senha
// estiverem errados.
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

// UPDATE: altera nome, e-mail e/ou senha do usuário logado, no MESMO registro.
// Recebe o ponteiro do usuário, então a mudança é feita na ficha original
// do vetor. Em cada pergunta, apertar só Enter mantém o valor atual.
// Devolve a mensagem que o feed vai mostrar.
const char *editarConta(Usuario *u) {
    char senhaAtual[20], novoNome[50], novoEmail[60], novaSenha[20];

    // Confirma que é a própria pessoa antes de alterar
    printf("\nSenha atual: ");
    lerTexto(senhaAtual, sizeof(senhaAtual));
    if (strcmp(u->senha, senhaAtual) != 0) {
        return "Senha incorreta. Nada foi alterado.";
    }

    printf("Novo nome (Enter para manter '%s'): ", u->nome);
    lerTexto(novoNome, sizeof(novoNome));
    printf("Novo e-mail (Enter para manter '%s'): ", u->email);
    lerTexto(novoEmail, sizeof(novoEmail));
    printf("Nova senha (Enter para manter a atual): ");
    lerTexto(novaSenha, sizeof(novaSenha));

    int mudou = 0;

    if (strlen(novoNome) > 0 && strcmp(novoNome, u->nome) != 0) {
        // O novo nome não pode pertencer a OUTRO usuário.
        int pos = buscarUsuario(novoNome);
        if (pos != -1 && &usuarios[pos] != u) {
            return "Esse nome ja esta em uso.";
        }

        // Posts e comentários guardam o autor como TEXTO (o nome). Se só
        // trocássemos o nome na conta, o que já foi publicado ficaria com o
        // nome velho. Por isso atualizamos o autor em todos eles.
        char nomeAntigo[50];
        strcpy(nomeAntigo, u->nome);
        strcpy(u->nome, novoNome);

        for (int i = 0; i < totalPosts; i++) {
            if (strcmp(posts[i].autor, nomeAntigo) == 0) {
                strcpy(posts[i].autor, novoNome);
            }
            for (int j = 0; j < posts[i].totalComentarios; j++) {
                if (strcmp(posts[i].comentarios[j].autor, nomeAntigo) == 0) {
                    strcpy(posts[i].comentarios[j].autor, novoNome);
                }
            }
        }
        mudou = 1;
    }

    if (strlen(novoEmail) > 0 && strcmp(novoEmail, u->email) != 0) {
        strcpy(u->email, novoEmail);
        mudou = 1;
    }

    if (strlen(novaSenha) > 0) {
        strcpy(u->senha, novaSenha);
        mudou = 1;
    }

    if (!mudou) return "Nenhuma alteracao feita.";

    salvarDados();   // grava o registro atualizado no arquivo
    return "Dados atualizados com sucesso!";
}

/* ============================================================
 * 8. CURTIDAS E SEGUIDORES (as tabelas de relacionamento)
 * ============================================================ */

// Procura a curtida de um usuário em um post. Devolve a posição ou -1.
int buscarCurtida(int usuarioId, int publicacaoId) {
    for (int i = 0; i < totalCurtidas; i++) {
        if (curtidas[i].usuarioId == usuarioId &&
            curtidas[i].publicacaoId == publicacaoId) {
            return i;
        }
    }
    return -1;
}

// Conta quantas curtidas um post tem.
int contarCurtidas(int publicacaoId) {
    int total = 0;
    for (int i = 0; i < totalCurtidas; i++) {
        if (curtidas[i].publicacaoId == publicacaoId) total++;
    }
    return total;
}

// CREATE ou DELETE: se a pessoa ainda não curtiu, cria a curtida; se já
// curtiu, remove (descurtir). Devolve a mensagem para o feed.
const char *alternarCurtida(Usuario *u, Publicacao *p) {
    int pos = buscarCurtida(u->id, p->id);

    if (pos != -1) {
        // Para "apagar" de um vetor, empurramos os itens seguintes uma
        // posição para trás (sobrescrevendo o apagado) e diminuímos o contador.
        for (int i = pos; i < totalCurtidas - 1; i++) {
            curtidas[i] = curtidas[i + 1];
        }
        totalCurtidas--;
        return "Curtida removida.";
    }

    if (totalCurtidas >= MAX_CURTIDAS) return "Limite de curtidas atingido.";

    curtidas[totalCurtidas].usuarioId = u->id;
    curtidas[totalCurtidas].publicacaoId = p->id;
    totalCurtidas++;
    return "Voce curtiu este post!";
}

// Procura "seguidor segue seguido". Devolve a posição ou -1.
int buscarSeguidor(int seguidorId, int seguidoId) {
    for (int i = 0; i < totalSeguidores; i++) {
        if (seguidores[i].seguidorId == seguidorId &&
            seguidores[i].seguidoId == seguidoId) {
            return i;
        }
    }
    return -1;
}

// Quantas pessoas seguem este usuário.
int contarSeguidores(int usuarioId) {
    int total = 0;
    for (int i = 0; i < totalSeguidores; i++) {
        if (seguidores[i].seguidoId == usuarioId) total++;
    }
    return total;
}

// Quantas pessoas este usuário segue.
int contarSeguindo(int usuarioId) {
    int total = 0;
    for (int i = 0; i < totalSeguidores; i++) {
        if (seguidores[i].seguidorId == usuarioId) total++;
    }
    return total;
}

// CREATE ou DELETE: segue ou deixa de seguir o autor do post.
const char *alternarSeguir(Usuario *u, Publicacao *p) {
    int posAutor = buscarUsuario(p->autor);
    if (posAutor == -1) return "Nao foi possivel encontrar o autor deste post.";

    Usuario *autor = &usuarios[posAutor];
    if (autor->id == u->id) return "Voce nao pode seguir a si mesmo.";

    int pos = buscarSeguidor(u->id, autor->id);
    if (pos != -1) {
        for (int i = pos; i < totalSeguidores - 1; i++) {
            seguidores[i] = seguidores[i + 1];
        }
        totalSeguidores--;
        return "Voce deixou de seguir este autor.";
    }

    if (totalSeguidores >= MAX_SEGUIDORES) return "Limite de seguidores atingido.";

    seguidores[totalSeguidores].seguidorId = u->id;
    seguidores[totalSeguidores].seguidoId = autor->id;
    totalSeguidores++;
    return "Agora voce segue este autor!";
}

/* ============================================================
 * 9. POSTS, PERFIL E FEED
 *
 * NOTA SOBRE PONTEIROS:
 * Quando passamos uma struct para uma função "normal", ela recebe
 * uma CÓPIA, e mudar a cópia não altera a original. Para alterar a
 * original, passamos o ENDEREÇO dela (&posts[atual]) e a função recebe
 * um PONTEIRO (Publicacao *p).
 * Para acessar um campo através de um ponteiro, usamos a seta:
 *     p->texto     (equivale a (*p).texto)
 * ============================================================ */

// Cria um post novo no final do vetor. Devolve 1 se deu certo, 0 se não.
// "const char *" significa: recebo um texto que a função não vai alterar.
int adicionarPost(const char *autor, const char *texto, const char *imagem) {
    if (totalPosts >= MAX_POSTS) return 0;

    Publicacao *p = &posts[totalPosts];
    p->id = proximoIdPost;
    proximoIdPost++;
    strcpy(p->autor, autor);
    strcpy(p->texto, texto);
    strcpy(p->imagem, imagem);

    if (strlen(imagem) > 0) {
        gerarDescricaoIA(imagem, p->descricaoImagem);
    } else {
        p->descricaoImagem[0] = '\0';   // texto vazio
    }

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

// CREATE: pergunta o texto (e a imagem) e publica.
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

// CREATE: adiciona um comentário ao post. Devolve a mensagem para o feed.
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

// DELETE: exclui o post da posição indicada. Só o autor pode excluir.
// Também apagamos as curtidas desse post, para não sobrar curtida
// apontando para um post que não existe mais.
const char *excluirPost(int posicao, Usuario *u) {
    if (strcmp(posts[posicao].autor, u->nome) != 0) {
        return "Voce so pode excluir os seus proprios posts.";
    }

    // 1) Remove as curtidas do post. Percorremos o vetor copiando para a
    //    frente só as curtidas que ficam ("novo" é a próxima posição livre).
    int idDoPost = posts[posicao].id;
    int novo = 0;
    for (int i = 0; i < totalCurtidas; i++) {
        if (curtidas[i].publicacaoId != idDoPost) {
            curtidas[novo] = curtidas[i];
            novo++;
        }
    }
    totalCurtidas = novo;

    // 2) Remove o post: empurra os seguintes uma posição para trás.
    //    Atribuir uma struct a outra (posts[i] = posts[i + 1]) copia todos
    //    os campos de uma vez, inclusive os comentários.
    for (int i = posicao; i < totalPosts - 1; i++) {
        posts[i] = posts[i + 1];
    }
    totalPosts--;

    return "Post excluido.";
}

// READ: mostra UM post na tela, com data, texto, curtidas e comentários.
// "logado" é o usuário que está olhando (para a fonte ampliada e para
// mostrar se ele já curtiu / já segue o autor).
void exibirPost(Publicacao *p, int posicao, Usuario *logado) {
    // Transforma o time_t em texto legível (dd/mm/aaaa hh:mm)
    char dataTexto[20];
    struct tm *t = localtime(&p->horario);
    strftime(dataTexto, sizeof(dataTexto), "%d/%m/%Y %H:%M", t);

    // Descobre se o usuário logado já segue o autor
    const char *tagSeguindo = "";
    int posAutor = buscarUsuario(p->autor);
    if (posAutor != -1 && buscarSeguidor(logado->id, usuarios[posAutor].id) != -1) {
        tagSeguindo = " [seguindo]";
    }

    printf("%s=== Post %d de %d ===%s\n", COR_TITULO, posicao + 1, totalPosts, COR_RESET);
    printf("Autor: %s%s  |  %s\n\n", p->autor, tagSeguindo, dataTexto);

    exibirTexto(p->texto, logado->fonteGrande);

    if (strlen(p->imagem) > 0) {
        printf("\n[Imagem anexada: %s]\n", p->imagem);
    }

    printf("\nCurtidas: %d%s\n", contarCurtidas(p->id),
           buscarCurtida(logado->id, p->id) != -1 ? " (voce curtiu)" : "");
    printf("Comentarios (%d):\n", p->totalComentarios);
    for (int i = 0; i < p->totalComentarios; i++) {
        printf("  - %s: %s\n", p->comentarios[i].autor, p->comentarios[i].texto);
    }
}

// READ: mostra os dados do usuário logado e as estatísticas dele.
void verPerfil(Usuario *u) {
    int meusPosts = 0, curtidasRecebidas = 0;
    for (int i = 0; i < totalPosts; i++) {
        if (strcmp(posts[i].autor, u->nome) == 0) {
            meusPosts++;
            curtidasRecebidas += contarCurtidas(posts[i].id);
        }
    }

    printf("\n%s=== Meu perfil ===%s\n", COR_TITULO, COR_RESET);
    printf("Id: %d\n", u->id);
    printf("Nome: %s\n", u->nome);
    printf("E-mail: %s\n", u->email);
    printf("Fonte ampliada: %s\n", u->fonteGrande ? "ligada" : "desligada");
    printf("Posts publicados: %d\n", meusPosts);
    printf("Curtidas recebidas: %d\n", curtidasRecebidas);
    printf("Seguidores: %d\n", contarSeguidores(u->id));
    printf("Seguindo: %d\n", contarSeguindo(u->id));

    // Lista os nomes de quem o usuário segue
    for (int i = 0; i < totalSeguidores; i++) {
        if (seguidores[i].seguidorId == u->id) {
            // os ids de usuário começam em 1, e a posição no vetor em 0
            for (int j = 0; j < totalUsuarios; j++) {
                if (usuarios[j].id == seguidores[i].seguidoId) {
                    printf("  - %s\n", usuarios[j].nome);
                }
            }
        }
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
        if (totalPosts > 0) {
            exibirPost(&posts[atual], atual, usuario);
        } else {
            printf("Ainda nao ha posts. Publique o primeiro!\n");
        }

        if (strlen(mensagem) > 0) {
            printf("\n%s>> %s%s\n", COR_OK, mensagem, COR_RESET);
            mensagem = "";
        }

        printf("\n--------------------------------------------\n");
        printf("1-Curtir/Descurtir   2-Comentar   3-Proximo   4-Anterior\n");
        printf("5-Ouvir post   6-Descrever imagem (IA)   7-Fonte ampliada\n");
        printf("8-Novo post   9-Editar minha conta   10-Excluir meu post\n");
        printf("11-Seguir/Deixar de seguir autor   12-Meu perfil\n");
        printf("0-Sair da conta\n> ");
        opcao = lerInteiro();

        // Sem posts, as opções que dependem de um post não funcionam.
        int precisaDePost = (opcao >= 1 && opcao <= 6) || opcao == 10 || opcao == 11;
        if (totalPosts == 0 && precisaDePost) {
            mensagem = "Nao ha posts para essa acao.";
            continue;   // volta ao início do laço
        }

        switch (opcao) {
            case 1:
                mensagem = alternarCurtida(usuario, &posts[atual]);
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
            case 9:   // UPDATE
                mensagem = editarConta(usuario);
                break;
            case 10:  // DELETE
                mensagem = excluirPost(atual, usuario);
                // se excluímos o último da lista, "atual" precisa recuar
                if (atual >= totalPosts) atual = totalPosts - 1;
                if (atual < 0) atual = 0;
                break;
            case 11:
                mensagem = alternarSeguir(usuario, &posts[atual]);
                break;
            case 12:  // READ
                verPerfil(usuario);
                pressionarEnter();
                break;
            case 0:
                break;   // o while abaixo encerra o laço
            default:
                mensagem = "Opcao invalida.";
        }
    } while (opcao != 0);

    salvarDados();   // guarda tudo: posts, curtidas, seguidores e preferências
}

/* ============================================================
 * 10. MAIN: menu inicial
 * ============================================================ */
int main() {
#ifdef _WIN32
    system("");   // truque para o terminal do Windows 10+ entender as cores ANSI
#endif

    carregarDados();                                   // lê o arquivo (se existir)
    if (totalUsuarios == 0) criarUsuariosExemplo();
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

