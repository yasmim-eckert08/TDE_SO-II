#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>

using namespace std;

struct Bloco {
    string arquivo;
    int proximo;
    int bytesUsados;
    bool livre;

    Bloco() : arquivo(""), proximo(-1), bytesUsados(0), livre(true) {}
};

struct Arquivo {
    int tamanhoBytes;
    vector<int> blocos;
    int blocoIndice; 
};

class Disco {
private:
    const int tamanhoBloco = 8;  // fixo
    vector<Bloco> blocos;
    unordered_map<string, Arquivo> tabelaArquivos;
    int metodoAlocacao;
    mt19937 gerador;

    int buscarEspacoContiguo(int blocosNecessarios) {
        int count = 0;
        for (int i = 0; i < (int)blocos.size(); i++) {
            if (blocos[i].livre) {
                count++;
                if (count == blocosNecessarios) return i - count + 1;
            } else count = 0;
        }
        return -1;
    }

    vector<int> buscarEspacoLivre(int n) {
        vector<int> livres;
        for (int i = 0; i < (int)blocos.size(); i++) {
            if (blocos[i].livre) livres.push_back(i);
        }
        return livres.size() >= n ? livres : vector<int>{};
    }

public:
    Disco(int tamanhoDiscoBytes, int metodo)
        : metodoAlocacao(metodo), gerador(random_device{}()) {
        int numBlocos = tamanhoDiscoBytes / tamanhoBloco;
        blocos.resize(numBlocos);
    }

    void mostrarMenu() {
        cout << "\nOperações disponíveis:\n"
             << "1 - Criar arquivo\n"
             << "2 - Estender arquivo\n"
             << "3 - Deletar arquivo\n"
             << "4 - Mostrar disco\n"
             << "5 - Mostrar tabela de diretório\n"
             << "6 - Sair\n"
             << "Escolha: ";
    }

    void criarArquivo() {
        string nome;
        int tamanhoBytes;
        cout << "Nome do arquivo: ";
        cin >> nome;
        if (tabelaArquivos.count(nome)) {
            cout << "Erro: arquivo já existe.\n";
            return;
        }
        cout << "Tamanho em bytes: ";
        cin >> tamanhoBytes;
        if (tamanhoBytes <= 0) {
            cout << "Tamanho inválido.\n";
            return;
        }
        int blocosNecessarios = (int)ceil((double)tamanhoBytes / tamanhoBloco);

        if (metodoAlocacao == 1) {
            // Contígua
            int inicio = buscarEspacoContiguo(blocosNecessarios);
            if (inicio == -1) {
                cout << "Erro: espaço insuficiente para arquivo contíguo.\n";
                return;
            }
            for (int i = inicio; i < inicio + blocosNecessarios; i++) {
                blocos[i].livre = false;
                blocos[i].arquivo = nome;
                blocos[i].bytesUsados = tamanhoBloco;
                blocos[i].proximo = -1;
            }

            int ultimo = inicio + blocosNecessarios - 1;
            int sobra = blocosNecessarios * tamanhoBloco - tamanhoBytes;
            blocos[ultimo].bytesUsados = tamanhoBloco - sobra;

            Arquivo arq;
            arq.tamanhoBytes = tamanhoBytes;
            arq.blocos.clear();
            for (int i = inicio; i < inicio + blocosNecessarios; i++) arq.blocos.push_back(i);
            arq.blocoIndice = -1;
            tabelaArquivos[nome] = arq;
            cout << "Arquivo '" << nome << "' criado. Tamanho: " << tamanhoBytes << " bytes. Fragmentação interna: " << sobra << " bytes.\n";
        } else if (metodoAlocacao == 2) {

            vector<int> livres = buscarEspacoLivre(blocosNecessarios);
            if ((int)livres.size() < blocosNecessarios) {
                cout << "Erro: espaço insuficiente para arquivo encadeado.\n";
                return;
            }
            shuffle(livres.begin(), livres.end(), gerador);
            livres.resize(blocosNecessarios);

            for (int i = 0; i < blocosNecessarios; i++) {
                int idx = livres[i];
                blocos[idx].livre = false;
                blocos[idx].arquivo = nome;
                blocos[idx].bytesUsados = tamanhoBloco;
                if (i == blocosNecessarios - 1)
                    blocos[idx].proximo = -1;
                else
                    blocos[idx].proximo = livres[i + 1];
            }

            int sobra = blocosNecessarios * tamanhoBloco - tamanhoBytes;
            blocos[livres.back()].bytesUsados = tamanhoBloco - sobra;

            Arquivo arq;
            arq.tamanhoBytes = tamanhoBytes;
            arq.blocos = livres;
            arq.blocoIndice = -1;
            tabelaArquivos[nome] = arq;
            cout << "Arquivo '" << nome << "' criado. Tamanho: " << tamanhoBytes << " bytes. Fragmentação interna: " << sobra << " bytes.\n";
        } else {

            int blocosNecessariosIndexado = blocosNecessarios + 1; // +1 bloco índice
            vector<int> livres = buscarEspacoLivre(blocosNecessariosIndexado);
            if ((int)livres.size() < blocosNecessariosIndexado) {
                cout << "Erro: espaço insuficiente para arquivo indexado.\n";
                return;
            }
            shuffle(livres.begin(), livres.end(), gerador);
            int blocoIndice = livres.back();
            livres.pop_back();

            Arquivo arq;
            arq.tamanhoBytes = tamanhoBytes;
            arq.blocoIndice = blocoIndice;
            arq.blocos = livres;

            blocos[blocoIndice].livre = false;
            blocos[blocoIndice].arquivo = nome;
            blocos[blocoIndice].bytesUsados = tamanhoBloco;
            blocos[blocoIndice].proximo = -1;

            for (int i = 0; i < blocosNecessarios; i++) {
                int idx = livres[i];
                blocos[idx].livre = false;
                blocos[idx].arquivo = nome;
                blocos[idx].bytesUsados = tamanhoBloco;
                blocos[idx].proximo = -1;
            }
            int sobra = blocosNecessarios * tamanhoBloco - tamanhoBytes;
            blocos[livres.back()].bytesUsados = tamanhoBloco - sobra;

            tabelaArquivos[nome] = arq;
            cout << "Arquivo '" << nome << "' criado. Tamanho: " << tamanhoBytes << " bytes. Fragmentação interna: " << sobra << " bytes.\n";
        }
    }

    void estenderArquivo() {
        string nome;
        int bytesExtras;
        cout << "Nome do arquivo para estender: ";
        cin >> nome;
        if (!tabelaArquivos.count(nome)) {
            cout << "Arquivo não encontrado.\n";
            return;
        }
        cout << "Quantidade de bytes para adicionar: ";
        cin >> bytesExtras;
        if (bytesExtras <= 0) {
            cout << "Quantidade inválida.\n";
            return;
        }
        Arquivo &arq = tabelaArquivos[nome];
        int blocosAtuais = (int)arq.blocos.size();
        int blocosNecessarios = (int)ceil((double)(arq.tamanhoBytes + bytesExtras) / tamanhoBloco);
        int blocosParaAdicionar = blocosNecessarios - blocosAtuais;

        if (blocosParaAdicionar == 0) {
            int novoTamanho = arq.tamanhoBytes + bytesExtras;
            int bytesNoUltimo = novoTamanho - (blocosAtuais - 1) * tamanhoBloco;
            blocos[arq.blocos.back()].bytesUsados = bytesNoUltimo;
            arq.tamanhoBytes = novoTamanho;
            cout << "Arquivo '" << nome << "' estendido. Novo tamanho: " << novoTamanho
                 << " bytes. Fragmentação interna: " << (blocosAtuais * tamanhoBloco - novoTamanho) << " bytes.\n";
            return;
        }

        vector<int> blocosLivres = buscarEspacoLivre(blocosParaAdicionar);
        if ((int)blocosLivres.size() < blocosParaAdicionar) {
            cout << "Erro: espaço insuficiente para extensão.\n";
            return;
        }

        shuffle(blocosLivres.begin(), blocosLivres.end(), gerador);
        vector<int> novosBlocos(blocosLivres.begin(), blocosLivres.begin() + blocosParaAdicionar);

        for (int idx : novosBlocos) {
            blocos[idx].livre = false;
            blocos[idx].arquivo = nome;
            blocos[idx].bytesUsados = tamanhoBloco;
            blocos[idx].proximo = -1;
        }

        if (metodoAlocacao == 2) {
            blocos[arq.blocos.back()].proximo = novosBlocos[0];
        } else if (metodoAlocacao == 1) {
            cout << "Estender arquivos contíguos não é suportado (precisa realocar).\n";
            return;
        } else {
            // indexada - nada a fazer, blocos já alocados
        }

        arq.blocos.insert(arq.blocos.end(), novosBlocos.begin(), novosBlocos.end());

        int novoTamanho = arq.tamanhoBytes + bytesExtras;
        int bytesNoUltimo = novoTamanho - (int(arq.blocos.size()) - 1) * tamanhoBloco;
        blocos[arq.blocos.back()].bytesUsados = bytesNoUltimo;
        arq.tamanhoBytes = novoTamanho;

        int frag = (int)(arq.blocos.size() * tamanhoBloco) - novoTamanho;
        cout << "Arquivo '" << nome << "' estendido. Novo tamanho: " << novoTamanho
             << " bytes. Fragmentação interna: " << frag << " bytes.\n";
    }

    void deletarArquivo() {
        string nome;
        cout << "Nome do arquivo a deletar: ";
        cin >> nome;
        if (!tabelaArquivos.count(nome)) {
            cout << "Arquivo não encontrado.\n";
            return;
        }
        Arquivo arq = tabelaArquivos[nome];
        if (metodoAlocacao == 3) {
            blocos[arq.blocoIndice].livre = true;
            blocos[arq.blocoIndice].arquivo = "";
            blocos[arq.blocoIndice].bytesUsados = 0;
            blocos[arq.blocoIndice].proximo = -1;
        }
        for (int b : arq.blocos) {
            blocos[b].livre = true;
            blocos[b].arquivo = "";
            blocos[b].bytesUsados = 0;
            blocos[b].proximo = -1;
        }
        tabelaArquivos.erase(nome);
        cout << "Arquivo '" << nome << "' deletado.\n";
    }

    void mostrarDisco() {
        cout << "\nDisco - Tamanho do bloco fixo: " << tamanhoBloco << " bytes\n";

        if (metodoAlocacao == 1) {
            for (int i = 0; i < (int)blocos.size(); i++) {
                cout << "[" << setw(2) << i << "] ";
                if (blocos[i].livre)
                    cout << "░ Livre\n";
                else {
                    cout << "█ " << blocos[i].arquivo;
                    // INICIO e FIM do arquivo
                    if (tabelaArquivos.count(blocos[i].arquivo)) {
                        const auto& arq = tabelaArquivos[blocos[i].arquivo];
                        if (i == arq.blocos.front()) cout << " → INÍCIO";
                        if (i == arq.blocos.back()) cout << " → FIM";
                    }
                    cout << " (bytes usados: " << blocos[i].bytesUsados << ")";
                    cout << "\n";
                }
            }
        } else if (metodoAlocacao == 2) {
            for (int i = 0; i < (int)blocos.size(); i++) {
                cout << "[" << setw(2) << i << "] ";
                if (blocos[i].livre)
                    cout << "░ Livre\n";
                else {
                    cout << "█ " << blocos[i].arquivo << " → ";
                    if (blocos[i].proximo == -1)
                        cout << "FIM";
                    else
                        cout << blocos[i].proximo;
                    cout << " (bytes usados: " << blocos[i].bytesUsados << ")";
                    cout << "\n";
                }
            }
        } else {
            // indexada
            for (int i = 0; i < (int)blocos.size(); i++) {
                cout << "[" << setw(2) << i << "] ";
                if (blocos[i].livre)
                    cout << "░ Livre\n";
                else {
                    bool isBlocoIndice = false;
                    string arquivo = blocos[i].arquivo;
                    if (tabelaArquivos.count(arquivo) && tabelaArquivos[arquivo].blocoIndice == i) {
                        isBlocoIndice = true;
                        cout << "█ " << arquivo << " → BLOCO ÍNDICE (Aponta para blocos: ";
                        for (int b : tabelaArquivos[arquivo].blocos) cout << b << " ";
                        cout << ")";
                    } else {
                        cout << "█ " << arquivo;
                    }
                    cout << " (bytes usados: " << blocos[i].bytesUsados << ")";
                    cout << "\n";
                }
            }
        }
    }

    void mostrarTabelaDiretorio() {
        cout << "\nTabela de Diretório:\n";
        cout << left << setw(15) << "Arquivo" << setw(15) << "Tamanho(bytes)" << "Blocos alocados\n";
        cout << "---------------------------------------------------\n";
        for (const auto& p : tabelaArquivos) {
            cout << left << setw(15) << p.first << setw(15) << p.second.tamanhoBytes;
            if (metodoAlocacao == 3)
                cout << p.second.blocoIndice << " ";
            for (int b : p.second.blocos) cout << b << " ";
            cout << "\n";
        }
    }
};

int main() {
    cout << "Simulador de alocação de arquivos\n";
    cout << "Tamanho fixo de bloco: 8 bytes\n";
    cout << "Tamanhos sugeridos para disco (bytes): 64, 512\n";

    int tamanhoDisco;
    do {
        cout << "Informe o tamanho total do disco em bytes (mínimo 16, máximo 1024, múltiplo de 8): ";
        cin >> tamanhoDisco;
    } while (tamanhoDisco < 16 || tamanhoDisco > 1024 || tamanhoDisco % 8 != 0);

    int metodo;
    do {
        cout << "Escolha o método de alocação:\n1 - Contígua\n2 - Encadeada\n3 - Indexada (i-nodes)\n";
        cin >> metodo;
    } while (metodo < 1 || metodo > 3);

    Disco disco(tamanhoDisco, metodo);

    while (true) {
        disco.mostrarMenu();
        int opcao;
        cin >> opcao;
        switch (opcao) {
            case 1: disco.criarArquivo(); break;
            case 2: disco.estenderArquivo(); break;
            case 3: disco.deletarArquivo(); break;
            case 4: disco.mostrarDisco(); break;
            case 5: disco.mostrarTabelaDiretorio(); break;
            case 6: cout << "Encerrando...\n"; return 0;
            default: cout << "Opção inválida.\n";
        }
    }

    return 0;
}