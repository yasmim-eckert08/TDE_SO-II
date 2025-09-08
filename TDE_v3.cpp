#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iomanip>
#include <random>
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

void displayEncadeado(const vector<int> &disk, const unordered_map<string, File> &files)
{
    cout << "Memória Encadeada:" << endl;
    unordered_map<int, tuple<string, int, int, int>> blockInfo;
    for (const auto &[nome, file] : files)
    {
        if (file.startBlock < 0 || file.startBlock >= static_cast<int>(disk.size()))
            continue;
        vector<int> chain;
        int cur = file.startBlock;
        int safety = 0;
        const int MAX_SAFETY = static_cast<int>(disk.size()) * 2;
        while (cur >= 0 && cur < static_cast<int>(disk.size()) && safety < MAX_SAFETY)
        {
            chain.push_back(cur);
            int nxt = disk[cur];
            if (nxt == -2)
                break;
            cur = nxt;
            ++safety;
        }
        if (chain.empty())
            continue;
        int fileBytes = 0;
        auto itSizes = fileSizesBytes.find(nome);
        if (itSizes != fileSizesBytes.end())
            fileBytes = itSizes->second;
        int chainSize = static_cast<int>(chain.size());
        for (int idx = 0; idx < chainSize; ++idx)
        {
            int bytesUsed = tamanhoBloco;
            if (idx == chainSize - 1)
            {
                int bytesBefore = (chainSize - 1) * tamanhoBloco;
                bytesUsed = fileBytes - bytesBefore;
                if (bytesUsed < 0)
                    bytesUsed = 0;
                if (bytesUsed > tamanhoBloco)
                    bytesUsed = tamanhoBloco;
            }
            blockInfo[chain[idx]] = make_tuple(nome, idx, chainSize, bytesUsed);
        }
    }
    int totalBytesLivres = 0;
    for (size_t i = 0; i < disk.size(); ++i)
    {
        auto itInfo = blockInfo.find(static_cast<int>(i));
        if (itInfo == blockInfo.end())
        {
            if (disk[i] == -1)
            {
                cout << "[" << i << "] ░" << endl;
                totalBytesLivres += tamanhoBloco;
            }
            else
            {
                cout << "[" << i << "] █ → ?" << endl;
            }
            continue;
        }
        auto [nome, pos, chainSize, bytesUsed] = itInfo->second;
        const File &file = files.at(nome);
        cout << "[" << i << "] " << file.color;
        for (int b = 0; b < bytesUsed; ++b)
            cout << "█";
        for (int b = bytesUsed; b < tamanhoBloco; ++b)
            cout << "░";
        cout << "\033[0m";
        int prox = disk[i];
        if (pos == 0)
        {
            if (prox == -2)
                cout << " → INICIO → FIM do " << file.name << endl;
            else
                cout << " → INICIO do " << file.name << " → [" << prox << "]" << endl;
        }
        else
        {
            if (prox == -2)
                cout << " → FIM do " << file.name << endl;
            else
                cout << " → [" << prox << "]" << endl;
        }
        totalBytesLivres += (tamanhoBloco - bytesUsed);
    }
    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
}

void displayIndexado(const vector<int> &disk, const unordered_map<string, File> &files)
{
    cout << "Memória Indexada:" << endl;
    unordered_map<int, tuple<string, int, int>> blockInfo;
    for (const auto &[nome, file] : files)
    {
        // bloco índice
        if (file.indexBlock >= 0 && file.indexBlock < static_cast<int>(disk.size()))
        {
            blockInfo[file.indexBlock] = make_tuple(nome, 1, tamanhoBloco);
        }
        // blocos de dados
        int fileBytes = 0;
        auto itSize = fileSizesBytes.find(nome);
        if (itSize != fileSizesBytes.end())
            fileBytes = itSize->second;
        int totalBlocks = static_cast<int>(file.dataBlocks.size());
        for (int idx = 0; idx < totalBlocks; ++idx)
        {
            int blk = file.dataBlocks[idx];
            if (blk < 0 || blk >= static_cast<int>(disk.size()))
                continue;
            int bytesUsed = tamanhoBloco;
            if (idx == totalBlocks - 1)
            {
                int before = (totalBlocks - 1) * tamanhoBloco;
                bytesUsed = fileBytes - before;
                if (bytesUsed < 0)
                    bytesUsed = 0;
                if (bytesUsed > tamanhoBloco)
                    bytesUsed = tamanhoBloco;
            }
            blockInfo[blk] = make_tuple(nome, 0, bytesUsed);
        }
    }
    int totalBytesLivres = 0;
    for (size_t i = 0; i < disk.size(); ++i)
    {
        auto itInfo = blockInfo.find(static_cast<int>(i));
        if (itInfo == blockInfo.end())
        {
            if (disk[i] == -1)
            {
                cout << "[" << i << "] ░" << endl;
                totalBytesLivres += tamanhoBloco;
            }
            else
            {
                cout << "[" << i << "] █ → ?" << endl;
            }
            continue;
        }
        auto [nome, tipo, bytesUsed] = itInfo->second;
        const File &file = files.at(nome);
        cout << "[" << i << "] " << file.color;
        for (int b = 0; b < bytesUsed; ++b)
            cout << "█";
        for (int b = bytesUsed; b < tamanhoBloco; ++b)
            cout << "░";
        cout << "\033[0m";
        if (tipo == 1)
        {
            cout << " → BLOCO ÍNDICE do " << file.name << " → [";
            for (size_t j = 0; j < file.dataBlocks.size(); ++j)
            {
                cout << file.dataBlocks[j];
                if (j < file.dataBlocks.size() - 1)
                    cout << ", ";
            }
            cout << "]";
        }
        totalBytesLivres += (tamanhoBloco - bytesUsed); // soma bytes livres no bloco
        cout << endl;
    }
    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
    // 64 - 22 () = 42 - 8 = 34 (bloco indice conta como bloco ocupado - 8 bytes)
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

void createArquivoContiguo(vector<int> &disk, unordered_map<string, File> &files, int &fileID)
{
    string fileName;
    int tamanhoBytes;
    cout << "Digite o nome do arquivo: ";
    cin >> fileName;
    if (files.find(fileName) != files.end())
    {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }
    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;
    if (tamanhoBlocos > static_cast<int>(disk.size()))
    {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }
    bool allocated = false;
    for (int i = 0; i <= static_cast<int>(disk.size()) - tamanhoBlocos; ++i)
    {
        bool canAllocate = true;
        for (int j = 0; j < tamanhoBlocos; ++j)
        {
            if (disk[i + j] != -1)
            {
                canAllocate = false;
                break;
            }
        }
        if (canAllocate)
        {
            File newFile;
            newFile.startBlock = i;
            newFile.size = tamanhoBlocos;
            newFile.name = fileName;
            newFile.color = getFileColor(fileID++);
            for (int j = 0; j < tamanhoBlocos; ++j)
            {
                disk[i + j] = i;
            }
            files[fileName] = newFile;
            tabelaDiretorio[fileName] = make_tuple(i, tamanhoBlocos);
            fileSizesBytes[fileName] = tamanhoBytes;
            cout << "Arquivo criado com sucesso!" << endl;
            displayContiguo(disk, files);
            allocated = true;
            break;
        }
    }
    if (!allocated)
    {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
    }
}

void createArquivoEncadeado(vector<int> &disk, unordered_map<string, File> &files, int &fileID)
{
    string fileName;
    int tamanhoBytes;
    cout << "Digite o nome do arquivo: ";
    cin >> fileName;
    if (files.find(fileName) != files.end())
    {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }
    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;
    if (tamanhoBlocos > static_cast<int>(disk.size()))
    {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }
    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i)
    {
        if (disk[i] == -1)
        {
            freeBlocks.push_back(i);
        }
    }
    if (freeBlocks.size() < static_cast<size_t>(tamanhoBlocos))
    {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
        return;
    }
    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);
    int prevBlock = -1;
    vector<int> dataBlocks;
    for (int i = 0; i < tamanhoBlocos; ++i)
    {
        int currentBlock = freeBlocks[i];
        dataBlocks.push_back(currentBlock);
        if (prevBlock != -1)
        {
            disk[prevBlock] = currentBlock;
        }
        prevBlock = currentBlock;
    }
    disk[prevBlock] = -2;
    string color = getFileColor(fileID++);
    File newFile;
    newFile.startBlock = dataBlocks[0];
    newFile.dataBlocks = dataBlocks;
    newFile.size = tamanhoBlocos;
    newFile.name = fileName;
    newFile.color = color;
    tabelaDiretorio[fileName] = make_tuple(dataBlocks[0], tamanhoBlocos);
    fileSizesBytes[fileName] = tamanhoBytes;
    files[fileName] = newFile;
    cout << "Arquivo criado com sucesso!" << endl;
    displayEncadeado(disk, files);
}

void createArquivoIndexado(vector<int> &disk, unordered_map<string, File> &files, int &fileID)
{
    string fileName;
    int tamanhoBytes;
    cout << "Digite o nome do arquivo: ";
    cin >> fileName;
    if (files.find(fileName) != files.end())
    {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }
    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;
    if (tamanhoBlocos > static_cast<int>(disk.size()))
    {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }
    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i)
    {
        if (disk[i] == -1)
        {
            freeBlocks.push_back(i);
        }
    }
    if (freeBlocks.size() < static_cast<size_t>(tamanhoBlocos + 1))
    {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
        return;
    }
    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);
    int indexBlock = freeBlocks.back();
    freeBlocks.pop_back();
    File newFile;
    newFile.indexBlock = indexBlock;
    newFile.name = fileName;
    newFile.size = tamanhoBlocos;
    newFile.color = getFileColor(fileID++);
    for (int i = 0; i < tamanhoBlocos; ++i)
    {
        newFile.dataBlocks.push_back(freeBlocks[i]);
        disk[freeBlocks[i]] = indexBlock;
    }
    disk[indexBlock] = -2;
    files[fileName] = newFile;
    tabelaDiretorio[fileName] = make_tuple(indexBlock, tamanhoBlocos);
    fileSizesBytes[fileName] = tamanhoBytes;
    cout << "Arquivo criado com sucesso!" << endl;
    displayIndexado(disk, files);
}

void deleteArquivo(vector<int> &disk, unordered_map<string, File> &filesIndexados,
                   unordered_map<string, File> &filesEncadeados, unordered_map<string, File> &filesContiguous)
{
    string fileName;
    cout << "Digite o nome do arquivo a ser deletado: ";
    cin >> fileName;
    if (filesContiguous.find(fileName) != filesContiguous.end())
    {
        File file = filesContiguous[fileName];
        for (int i = file.startBlock; i < file.startBlock + file.size; ++i)
        {
            disk[i] = -1;
        }
        filesContiguous.erase(fileName);
        tabelaDiretorio.erase(fileName);
        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    }
    else if (filesEncadeados.find(fileName) != filesEncadeados.end())
    {
        File file = filesEncadeados[fileName];
        for (int block : file.dataBlocks)
        {
            disk[block] = -1;
        }
        disk[file.startBlock] = -1;
        filesEncadeados.erase(fileName);
        tabelaDiretorio.erase(fileName);
        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    }
    else if (filesIndexados.find(fileName) != filesIndexados.end())
    {
        File file = filesIndexados[fileName];
        disk[file.indexBlock] = -1;
        for (int block : file.dataBlocks)
        {
            disk[block] = -1;
        }
        filesIndexados.erase(fileName);
        tabelaDiretorio.erase(fileName);
        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    }
    else
    {
        cout << "Erro: Arquivo não encontrado!" << endl;
    }
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