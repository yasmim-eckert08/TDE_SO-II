#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iomanip>
using namespace std;

const int tamanhoBloco = 8; // bytes fixos por bloco

// juntando struct Arquivo com struct Bloco em uma só struct
struct File {
    int indexBlock = -1;
    int startBlock = -1;
    vector<int> dataBlocks;
    int size = 0;
    string name;
    string color;
    int fragmentacao = 0;
};

string getFileColor(int fileID) {
    static const vector<string> colors = {
        "\033[95m", // rosa
        "\033[32m", // verde
        "\033[34m", // azul
        "\033[31m", // vermelho
        "\033[36m", // ciano
        "\033[33m", // amarelo
        "\033[35m", // magenta
    };
    return colors[fileID % colors.size()];
}

unordered_map<string, tuple<int, int>> tabelaDiretorio;

// map para armazenar tamanho real do arquivo em bytes
unordered_map<string, int> fileSizesBytes;

// separando cada alocação em funções diferentes
void displayContiguo(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Contígua:" << endl;

    int totalBytesLivres = 0;

    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            cout << "[" << i << "] ░" << endl;
            totalBytesLivres += tamanhoBloco;
        } else {
            int startBlock = disk[i];
            auto it = find_if(files.begin(), files.end(), [startBlock](const auto& p) {
                return p.second.startBlock == startBlock;
            });
            if (it == files.end()) {
                cout << "[" << i << "] ?" << endl;
                continue;
            }
            const File& file = it->second;

            int bytesUsed = tamanhoBloco;
            auto itSizes = fileSizesBytes.find(file.name);
            if (itSizes != fileSizesBytes.end()) {
                int totalBytes = itSizes->second;
                int lastBlockIndex = file.startBlock + file.size - 1;
                if (static_cast<int>(i) == lastBlockIndex) {
                    int bytesBefore = (file.size - 1) * tamanhoBloco;
                    bytesUsed = totalBytes - bytesBefore;
                    if (bytesUsed < 0) bytesUsed = 0;
                    if (bytesUsed > tamanhoBloco) bytesUsed = tamanhoBloco;
                }
            }

            cout << "[" << i << "] " << file.color;
            for (int b = 0; b < bytesUsed; ++b) cout << "█";
            for (int b = bytesUsed; b < tamanhoBloco; ++b) cout << "░";
            cout << "\033[0m";

            if (static_cast<int>(i) == file.startBlock)
                cout << " → INICIO do " << file.name << endl;
            else if (static_cast<int>(i) == file.startBlock + file.size - 1)
                cout << " → FIM do " << file.name << endl;
            else
                cout << " [" << file.name << "]" << endl;

            totalBytesLivres += (tamanhoBloco - bytesUsed);
        }
    }

    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
}

void displayTabelaDiretorios() {
    cout << "\nTabela de Diretórios:\n";
    cout << left << setw(18) << "Nome do arquivo"
         << setw(15) << "Bloco inicial"
         << setw(18) << "Tamanho (blocos)"
         << setw(20) << "Tamanho total (bytes)"
         << setw(25) << "Fragmentação interna (bytes)"
         << "\n---------------------------------------------------------------\n";

    for (const auto& [nome, arq] : tabelaArquivos) {
        int tamanhoTotalBytes = 0;
        int fragmentacaoInterna = 0;
        for (int b : arq.blocos) {
            tamanhoTotalBytes += blocos[b].bytesUsados;
            fragmentacaoInterna += (tamanhoBloco - blocos[b].bytesUsados);
        }
        cout << left << setw(18) << nome
             << setw(15) << arq.blocos.front()
             << setw(18) << (int)arq.blocos.size()
             << setw(20) << tamanhoTotalBytes
             << setw(25) << fragmentacaoInterna
             << "\n";
    }
}

void criarArquivoContiguo() {
    string nomeArquivo;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> nomeArquivo;

    if (tabelaArquivos.count(nomeArquivo)) {
        cout << "Erro: Arquivo já existe!\n";
        return;
    }

    cout << "Digite o tamanho do arquivo (em bytes): ";
    cin >> tamanhoBytes;

    int blocosNecessarios = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;

    if (blocosNecessarios > (int)blocos.size()) {
        cout << "Erro: Arquivo maior que o disco!\n";
        return;
    }

    // Buscar espaço contíguo suficiente
    int inicio = -1;
    int count = 0;
    for (int i = 0; i < (int)blocos.size(); i++) {
        if (blocos[i].livre) {
            if (inicio == -1) inicio = i;
            count++;
            if (count == blocosNecessarios) break;
        } else {
            inicio = -1;
            count = 0;
        }
    }

    if (count < blocosNecessarios) {
        cout << "Erro: Espaço contíguo insuficiente!\n";
        return;
    }

    // Alocar blocos
    Arquivo arq;
    for (int i = inicio; i < inicio + blocosNecessarios; i++) {
        blocos[i].livre = false;
        blocos[i].arquivo = nomeArquivo;
        blocos[i].bytesUsados = (i == inicio + blocosNecessarios - 1) ? (tamanhoBytes % tamanhoBloco) : tamanhoBloco;
        if (blocos[i].bytesUsados == 0) blocos[i].bytesUsados = tamanhoBloco;
        arq.blocos.push_back(i);
    }

    tabelaArquivos[nomeArquivo] = arq;
    cout << "Arquivo '" << nomeArquivo << "' criado com sucesso.\n";
}

void criarArquivoEncadeado() {
    string nomeArquivo;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> nomeArquivo;

    if (tabelaArquivos.count(nomeArquivo)) {
        cout << "Erro: Arquivo já existe!\n";
        return;
    }

    cout << "Digite o tamanho do arquivo (em bytes): ";
    cin >> tamanhoBytes;

    int blocosNecessarios = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;

    if (blocosNecessarios > (int)blocos.size()) {
        cout << "Erro: Arquivo maior que o disco!\n";
        return;
    }

    // Buscar blocos livres aleatoriamente (não precisam ser adjacentes)
    vector<int> blocosLivres;
    for (int i = 0; i < (int)blocos.size(); i++)
        if (blocos[i].livre)
            blocosLivres.push_back(i);

    if ((int)blocosLivres.size() < blocosNecessarios) {
        cout << "Erro: Espaço insuficiente!\n";
        return;
    }

    Arquivo arq;
    // Alocar blocos e setar ponteiros
    for (int i = 0; i < blocosNecessarios; i++) {
        int b = blocosLivres[i];
        blocos[b].livre = false;
        blocos[b].arquivo = nomeArquivo;
        blocos[b].bytesUsados = (i == blocosNecessarios - 1) ? (tamanhoBytes % tamanhoBloco) : tamanhoBloco;
        if (blocos[b].bytesUsados == 0) blocos[b].bytesUsados = tamanhoBloco;
        blocos[b].proximo = (i == blocosNecessarios - 1) ? -1 : blocosLivres[i + 1];
        arq.blocos.push_back(b);
    }

    tabelaArquivos[nomeArquivo] = arq;
    cout << "Arquivo '" << nomeArquivo << "' criado com sucesso.\n";
}

void criarArquivoIndexado() {
    string nomeArquivo;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> nomeArquivo;

    if (tabelaArquivos.count(nomeArquivo)) {
        cout << "Erro: Arquivo já existe!\n";
        return;
    }

    cout << "Digite o tamanho do arquivo (em bytes): ";
    cin >> tamanhoBytes;

    int blocosNecessarios = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;

    // Indexada -> blocosNecessarios + 1 bloco para índice
    if (blocosNecessarios + 1 > (int)blocos.size()) {
        cout << "Erro: Arquivo maior que o disco!\n";
        return;
    }

    // Buscar blocos livres
    vector<int> blocosLivres;
    for (int i = 0; i < (int)blocos.size(); i++)
        if (blocos[i].livre)
            blocosLivres.push_back(i);

    if ((int)blocosLivres.size() < blocosNecessarios + 1) {
        cout << "Erro: Espaço insuficiente!\n";
        return;
    }

    Arquivo arq;
    arq.blocoIndice = blocosLivres.back();
    blocosLivres.pop_back();

    blocos[arq.blocoIndice].livre = false;
    blocos[arq.blocoIndice].arquivo = nomeArquivo;
    blocos[arq.blocoIndice].bytesUsados = tamanhoBloco; // índice ocupa o bloco todo
    blocos[arq.blocoIndice].proximo = -1;

    for (int i = 0; i < blocosNecessarios; i++) {
        int b = blocosLivres[i];
        blocos[b].livre = false;
        blocos[b].arquivo = nomeArquivo;
        blocos[b].bytesUsados = (i == blocosNecessarios - 1) ? (tamanhoBytes % tamanhoBloco) : tamanhoBloco;
        if (blocos[b].bytesUsados == 0) blocos[b].bytesUsados = tamanhoBloco;
        blocos[b].proximo = -1;
        arq.blocos.push_back(b);
    }

    tabelaArquivos[nomeArquivo] = arq;
    cout << "Arquivo '" << nomeArquivo << "' criado com sucesso.\n";
}

void deletarArquivo() {
    string nomeArquivo;
    cout << "Digite o nome do arquivo para deletar: ";
    cin >> nomeArquivo;

    if (!tabelaArquivos.count(nomeArquivo)) {
        cout << "Erro: Arquivo não encontrado!\n";
        return;
    }
    Arquivo arq = tabelaArquivos[nomeArquivo];

    for (int b : arq.blocos) {
        blocos[b].livre = true;
        blocos[b].arquivo = "";
        blocos[b].bytesUsados = 0;
        blocos[b].proximo = -1;
    }

    if (metodoAlocacao == 3 && arq.blocoIndice != -1) {
        blocos[arq.blocoIndice].livre = true;
        blocos[arq.blocoIndice].arquivo = "";
        blocos[arq.blocoIndice].bytesUsados = 0;
        blocos[arq.blocoIndice].proximo = -1;
    }

    tabelaArquivos.erase(nomeArquivo);
    cout << "Arquivo '" << nomeArquivo << "' deletado com sucesso!\n";
}

int main() {
    cout << "Escolha o tamanho do disco em bytes (bloco com 8 bytes): 32, 64, 512): ";
    int tamanhoDisco;
    cin >> tamanhoDisco;

    while (tamanhoDisco < 5 || tamanhoDisco > 1000) {
        cout << "Tamanho inválido. Escolha entre 5 e 1000 blocos: ";
        cin >> tamanhoDisco;
    }

    blocos = vector<Bloco>(tamanhoDisco);

    cout << "Escolha o método de alocação:\n1 - Contígua\n2 - Encadeada\n3 - Indexada\n";
    cin >> metodoAlocacao;

    int opcao;
    do {
        cout << "\nMenu:\n";
        cout << "1 - Criar arquivo\n";
        cout << "2 - Deletar arquivo\n";
        cout << "3 - Mostrar disco\n";
        cout << "4 - Mostrar tabela de diretórios\n";
        cout << "5 - Sair\n";
        cout << "Escolha uma opção: ";
        cin >> opcao;

        switch (opcao) {
            case 1:
                if (metodoAlocacao == 1) criarArquivoContiguo();
                else if (metodoAlocacao == 2) criarArquivoEncadeado();
                else if (metodoAlocacao == 3) criarArquivoIndexado();
                break;
            case 2:
                deletarArquivo();
                break;
            case 3:
                mostrarDisco();
                break;
            case 4:
                displayTabelaDiretorios();
                break;
            case 5:
                cout << "Saindo...\n";
                break;
            default:
                cout << "Opção inválida!\n";
        }
    } while (opcao != 5);

    return 0;
}