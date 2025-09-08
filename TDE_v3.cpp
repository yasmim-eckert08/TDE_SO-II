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
struct File
{
    int indexBlock = -1;
    int startBlock = -1;
    vector<int> dataBlocks;
    int size = 0;
    string name;
    string color;
    int fragmentacao = 0;
};

string getFileColor(int fileID)
{
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
void displayContiguo(const vector<int> &disk, const unordered_map<string, File> &files)
{
    cout << "Memória Contígua:" << endl;

    int totalBytesLivres = 0;

    for (size_t i = 0; i < disk.size(); ++i)
    {
        if (disk[i] == -1)
        {
            cout << "[" << i << "] ░" << endl;
            totalBytesLivres += tamanhoBloco;
        }
        else
        {
            int startBlock = disk[i];
            auto it = find_if(files.begin(), files.end(), [startBlock](const auto &p)
                              { return p.second.startBlock == startBlock; });
            if (it == files.end())
            {
                cout << "[" << i << "] ?" << endl;
                continue;
            }
            const File &file = it->second;

            int bytesUsed = tamanhoBloco;
            auto itSizes = fileSizesBytes.find(file.name);
            if (itSizes != fileSizesBytes.end())
            {
                int totalBytes = itSizes->second;
                int lastBlockIndex = file.startBlock + file.size - 1;
                if (static_cast<int>(i) == lastBlockIndex)
                {
                    int bytesBefore = (file.size - 1) * tamanhoBloco;
                    bytesUsed = totalBytes - bytesBefore;
                    if (bytesUsed < 0)
                        bytesUsed = 0;
                    if (bytesUsed > tamanhoBloco)
                        bytesUsed = tamanhoBloco;
                }
            }

            cout << "[" << i << "] " << file.color;
            for (int b = 0; b < bytesUsed; ++b)
                cout << "█";
            for (int b = bytesUsed; b < tamanhoBloco; ++b)
                cout << "░";
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

void displayDiretorioContiguo(const unordered_map<string, File> &files)
{
    cout << "\nTabela de Diretório - Alocação Contígua:\n";
    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(15) << "Bloco Inicial" << "| "
         << setw(17) << "Tamanho (blocos)" << "| "
         << setw(15) << "Tamanho (bytes)" << "| "
         << setw(12) << "Fragmentação Interna (bytes)" << "\n";
    cout << string(20 + 15 + 17 + 15 + 12 + 30, '-') << "\n";
    for (const auto &[nome, file] : files)
    {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);
        if (it != fileSizesBytes.end())
        {
            tamanhoBytes = it->second;
        }
        int fragmentacao = (file.size * tamanhoBloco) - tamanhoBytes;
        cout << left << setw(20) << nome << "| "
             << right << setw(15) << file.startBlock << "| "
             << right << setw(17) << file.size << "| "
             << right << setw(15) << tamanhoBytes << "| "
             << right << setw(12) << fragmentacao << "\n";
    }
}
void displayDiretorioEncadeado(const unordered_map<string, File> &files)
{
    cout << "\nTabela de Diretório - Alocação Encadeada:\n";
    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(15) << "Bloco Inicial" << "| "
         << setw(17) << "Tamanho (blocos)" << "| "
         << setw(15) << "Tamanho (bytes)" << "| "
         << setw(22) << "Fragmentação Interna (bytes)" << "\n";
    cout << string(20 + 15 + 17 + 15 + 22 + 4 * 2 + 18, '-') << "\n";
    for (const auto &[nome, file] : files)
    {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);
        if (it != fileSizesBytes.end())
        {
            tamanhoBytes = it->second;
        }
        int fragmentacao = (file.size * tamanhoBloco) - tamanhoBytes;
        cout << left << setw(20) << nome << "| "
             << right << setw(15) << file.startBlock << "| "
             << right << setw(17) << file.size << "| "
             << right << setw(15) << tamanhoBytes << "| "
             << right << setw(22) << fragmentacao << "\n";
        cout << "Blocos Encadeados: [";
        for (size_t i = 0; i < file.dataBlocks.size(); ++i)
        {
            cout << file.dataBlocks[i];
            if (i < file.dataBlocks.size() - 1)
                cout << " → ";
        }
        cout << "]\n";
    }
}
void displayDiretorioIndexado(const unordered_map<string, File> &files)
{
    cout << "\nTabela de Diretório - Alocação Indexada:\n";
    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(16) << "Bloco Índice" << "| "
         << setw(19) << "Tamanho (blocos)" << "| "
         << setw(19) << "Tamanho (bytes)" << "| "
         << setw(28) << "Fragmentação Interna (bytes)" << "\n";
    cout << string(20 + 15 + 19 + 19 + 28 + 12, '-') << "\n";
    for (const auto &[nome, file] : files)
    {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);
        if (it != fileSizesBytes.end())
        {
            tamanhoBytes = it->second;
        }
        int fragmentacao = (file.size * tamanhoBloco) - tamanhoBytes;
        cout << left << setw(20) << nome << "| "
             << right << setw(15) << file.indexBlock << "| "
             << right << setw(19) << file.size << "| "
             << right << setw(19) << tamanhoBytes << "| "
             << right << setw(28) << fragmentacao << "\n";
        cout << "Blocos de Dados: [";
        for (size_t i = 0; i < file.dataBlocks.size(); ++i)
        {
            cout << file.dataBlocks[i];
            if (i < file.dataBlocks.size() - 1)
                cout << ", ";
        }
        cout << "]\n";
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

int main()
{
    int diskSizeBytes;
    do
    {
        cout << "Determine o tamanho do disco em bytes (mínimo 16 (2 blocos) | máximo 1024 bytes (128 blocos)): ";
        cin >> diskSizeBytes;
        if (diskSizeBytes < 16 || diskSizeBytes > 1024)
        {
            cout << "Tamanho inválido! Digite novamente, mínimo 16 e máximo 1024.\n";
        }
    } while (diskSizeBytes < 16 || diskSizeBytes > 1024);
    int diskSizeBlocks = (diskSizeBytes + tamanhoBloco - 1) / tamanhoBloco;
    cout << "Disco de " << diskSizeBytes << " bytes criado com " << diskSizeBlocks << "blocos de " << tamanhoBloco << " bytes.\n ";
    vector<int>
        disk(diskSizeBlocks, -1);
    cout << "Estado inicial do disco:" << endl;
    for (int i = 0; i < diskSizeBlocks; ++i)
    {
        cout << "[" << i << "] ░" << endl;
    }
    int fileID = 0;
    unordered_map<string, File> filesContiguous;
    unordered_map<string, File> filesEncadeados;
    unordered_map<string, File> filesIndexados;
    int tipoAlocacao;
    cout << "Escolha o tipo de alocação:\n1. Contígua\n2. Encadeada\n3. Indexada\n";
    cin >> tipoAlocacao;
    while (true)
    {
        cout << "\nAgora, selecione uma das opções:\n";
        cout << "1. Criar arquivo\n";
        cout << "2. Deletar arquivo\n";
        cout << "3. Mostrar disco\n";
        cout << "4. Mostrar tabela de diretório\n";
        cout << "5. Encerrar o programa\n";
        int opcao;
        cin >> opcao;
        switch (opcao)
        {
        case 1:
            if (tipoAlocacao == 1)
            {
                createArquivoContiguo(disk, filesContiguous, fileID);
            }
            else if (tipoAlocacao == 2)
            {
                createArquivoEncadeado(disk, filesEncadeados, fileID);
            }
            else if (tipoAlocacao == 3)
            {
                createArquivoIndexado(disk, filesIndexados, fileID);
            }
            break;
        case 2:
            deleteArquivo(disk, filesIndexados, filesEncadeados, filesContiguous);
            break;
        case 3:
            if (tipoAlocacao == 1)
            {
                displayContiguo(disk, filesContiguous);
            }
            else if (tipoAlocacao == 2)
            {
                displayEncadeado(disk, filesEncadeados);
            }
            else if (tipoAlocacao == 3)
            {
                displayIndexado(disk, filesIndexados);
            }
            break;
        case 4:
            if (tipoAlocacao == 1)
            {
                displayDiretorioContiguo(filesContiguous);
            }
            else if (tipoAlocacao == 2)
            {
                displayDiretorioEncadeado(filesEncadeados);
            }
            else if (tipoAlocacao == 3)
            {
                displayDiretorioIndexado(filesIndexados);
            }
            break;
        case 5:
            cout << "Encerrando o programa..." << endl;
            return 0;
        default:
            cout << "Opção inválida! Tente novamente!" << endl;
        }
    }
    return 0;
}