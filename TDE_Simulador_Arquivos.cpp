#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <iomanip>
#include <random>
using namespace std;

const int tamanhoBloco = 8; // 8 bytes por bloco (valor fixo)

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
    // função que retorna uma cor ANSI para o arquivo com base no seu ID
    static const vector<string> colors = {
        "\033[95m", // rosa
        "\033[32m", // verde
        "\033[34m", // azul
        "\033[31m", // vermelho
        "\033[36m", // ciano
        "\033[33m", // amarelo
        "\033[35m", // magenta
    };
    // retorna a cor correspondente usando mod (%) para alternar entre as cores
    return colors[fileID % colors.size()];
}

// map que associa cada arquivo a uma tuple (startBlock, size) representando a tabela de diretório
unordered_map<string, tuple<int, int>> tabelaDiretorio;

// map para armazenar o tamanho real de cada arquivo em bytes
unordered_map<string, int> fileSizesBytes;

// display no terminal para cada método de alocação
void displayContiguo(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Contígua:" << endl;

    int totalBytesLivres = 0;

    for (size_t i = 0; i < disk.size(); ++i) { // percorre cada bloco do disco
        if (disk[i] == -1) { // bloco livre
            cout << "[" << i << "] ░" << endl;
            totalBytesLivres += tamanhoBloco;
        } else { // bloco ocupado
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

            // verifica o tamanho real do arquivo para calcular a fragmentação interna, ajustando a visualização para blocos parcialmente preenchidos 
            auto itSizes = fileSizesBytes.find(file.name);
            
            if (itSizes != fileSizesBytes.end()) {
                int totalBytes = itSizes -> second;
                int lastBlockIndex = file.startBlock + file.size - 1;
                
                if (static_cast<int>(i) == lastBlockIndex) {
                    int bytesBefore = (file.size - 1) * tamanhoBloco;
                    bytesUsed = totalBytes - bytesBefore;
                    if (bytesUsed < 0) 
                    bytesUsed = 0;
                    if (bytesUsed > tamanhoBloco) 
                    bytesUsed = tamanhoBloco;
                }
            }

            // imprime o bloco com cor e caracteres representando bytes usados e livres
            cout << "[" << i << "] " << file.color;
            
            for (int b = 0; b < bytesUsed; ++b) 
                cout << "█"; // bytes ocupados
            
            for (int b = bytesUsed; b < tamanhoBloco; ++b) 
                cout << "░"; // bytes livres no bloco
            
            cout << "\033[0m"; // reset da cor

            // indica posição do bloco dentro do arquivo
            if (file.size == 1) {
                cout << " → INICIO/FIM do " << file.name << endl;
            }
            else if (static_cast<int>(i) == file.startBlock) {
                cout << " → INICIO do " << file.name << endl;
            }
            else if (static_cast<int>(i) == file.startBlock + file.size - 1) {
                cout << " → FIM do " << file.name << endl;
            }
            else {
                cout << " [" << file.name << "]" << endl;
            }

            totalBytesLivres += (tamanhoBloco - bytesUsed); // calcula fragmentação interna
        }
    }

    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
}

void displayEncadeado(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Encadeada:" << endl;
    
    // map que associa cada bloco a uma tuple: (nome do arquivo, posição na cadeia, tamanho da cadeia, bytes usados)
    unordered_map<int, tuple<string, int, int, int>> blockInfo;

    for (const auto &[nome, file] : files) {
        // para cada arquivo, cria a cadeia de blocos
        if (file.startBlock < 0 || file.startBlock >= static_cast<int>(disk.size()))
            continue;
        
        vector<int> chain;
        int cur = file.startBlock;
        int safety = 0;
        const int MAX_SAFETY = static_cast<int>(disk.size()) * 2;
        
        // limite de segurança para evitar loops infinitos (em caso de corrupção dos ponteiros)
        while (cur >= 0 && cur < static_cast<int>(disk.size()) && safety < MAX_SAFETY) {
            chain.push_back(cur); // adiciona o bloco atual à cadeia
            int nxt = disk[cur]; // próximo bloco na cadeia
            if (nxt == -2) // fim da cadeia/arquivo
                break;
            cur = nxt;
            ++safety;
        }

        if (chain.empty())
            continue;
        
        int fileBytes = 0;
        auto itSizes = fileSizesBytes.find(nome);
        
        // obtém o tamanho real do arquivo em bytes
        if (itSizes != fileSizesBytes.end())
            fileBytes = itSizes->second;
        
        int chainSize = static_cast<int>(chain.size());
        
        for (int idx = 0; idx < chainSize; ++idx) {
            int bytesUsed = tamanhoBloco;
            
            // calcula bytes usados no último bloco da cadeia para visualização correta e parcial do bloco
            if (idx == chainSize - 1) {
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

    // percorre todos os blocos do disco para exibir
    for (size_t i = 0; i < disk.size(); ++i) {
        auto itInfo = blockInfo.find(static_cast<int>(i));
        
        if (itInfo == blockInfo.end()) {
            // bloco livre ou desconhecidos
            if (disk[i] == -1) {
                cout << "[" << i << "] ░" << endl;
                totalBytesLivres += tamanhoBloco;
            } else {
                cout << "[" << i << "] █ → ?" << endl;
            }
            continue;
        }

        auto [nome, pos, chainSize, bytesUsed] = itInfo->second;
        const File &file = files.at(nome);

        // imprime bloco com cores e bytes usados/livres
        cout << "[" << i << "] " << file.color;
        for (int b = 0; b < bytesUsed; ++b)
            cout << "█";

        for (int b = bytesUsed; b < tamanhoBloco; ++b)
            cout << "░";
        
        cout << "\033[0m"; // reset da cor

        int prox = disk[i]; // próximo bloco na cadeia
        
        if (chainSize == 1) {
            cout << " → INICIO/FIM do " << file.name << endl;
        } else if (pos == 0) {
            if (prox == -2) 
                cout << " → INICIO → FIM do " << file.name << endl;
            else  
                cout << " → INICIO do " << file.name << " → [" << prox << "]" << endl;
        }
        else {
            if (prox == -2) 
                cout << " → FIM do " << file.name << endl;
            else 
                cout << " → [" << prox << "]" << endl;
        }

        totalBytesLivres += (tamanhoBloco - bytesUsed); // calcula fragmentação interna
    }

    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
}

void displayIndexado(const vector<int> &disk, const unordered_map<string, File>& files) {
    cout << "Memória Indexada:" << endl;

    // map que associa cada bloco a uma tuple: (nome do arquivo, tipo do bloco, bytes usados)
    unordered_map<int, tuple<string, int, int>> blockInfo;

    for (const auto &[nome, file] : files) {
        // registra o bloco indice
        if (file.indexBlock >= 0 && file.indexBlock < static_cast<int>(disk.size())) {
            blockInfo[file.indexBlock] = make_tuple(nome, 1, tamanhoBloco);
        }

        // obtém o tamanho real do arquivo em bytes
        int fileBytes = 0;
        auto itSize = fileSizesBytes.find(nome);

        if (itSize != fileSizesBytes.end())
            fileBytes = itSize->second;
        
        // registra cada bloco de dados do arquivo
        int totalBlocks = static_cast<int>(file.dataBlocks.size());
        
        for (int idx = 0; idx < totalBlocks; ++idx) {
            int blk = file.dataBlocks[idx];
            if (blk < 0 || blk >= static_cast<int>(disk.size()))
                continue;
            int bytesUsed = tamanhoBloco;
            
            if (idx == totalBlocks - 1) {
                // último bloco - calcula bytes usados para visualização correta
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

    for (size_t i = 0; i < disk.size(); ++i) {
        // percorre cada bloco do disco para exibir
        auto itInfo = blockInfo.find(static_cast<int>(i));
        
        if (itInfo == blockInfo.end()) {
            // bloco livres ou desconhecidos
            if (disk[i] == -1) {
                cout << "[" << i << "] ░" << endl;
                totalBytesLivres += tamanhoBloco; 
            } else {
                cout << "[" << i << "] █ → ?" << endl;
            }
            continue;
        }

        auto [nome, tipo, bytesUsed] = itInfo->second;
        const File& file = files.at(nome);

        // imprime bloco com cores e bytes usados/livres
        cout << "[" << i << "] " << file.color;
        for (int b = 0; b < bytesUsed; ++b)
            cout << "█";
        for (int b = bytesUsed; b < tamanhoBloco; ++b)
            cout << "░";
        
        cout << "\033[0m"; // reset da cor
        
        if (tipo == 1) { 
            // bloco índice → mostra os ponteiros para os blocos de dados
            cout << " → BLOCO ÍNDICE do " << file.name << " → [";
            for (size_t j = 0; j < file.dataBlocks.size(); ++j) {
                cout << file.dataBlocks[j];
                if (j < file.dataBlocks.size() - 1) cout << ", ";
            }
            cout << "]";
        } else {
            // bloco de dados → indica início, meio ou fim do arquivo
            int totalBlocks = static_cast<int>(file.dataBlocks.size());
            if (totalBlocks == 1) {
                cout << " → INICIO/FIM do " << file.name;
            } else {
                int idx = find(file.dataBlocks.begin(), file.dataBlocks.end(), (int)i) - file.dataBlocks.begin();
                if (idx == 0) {
                    cout << " → INICIO do " << file.name;
                } else if (idx == totalBlocks - 1) {
                    cout << " → FIM do " << file.name;
                } else {
                    cout << " [" << file.name << "]";
                }
            }
        }
        
        totalBytesLivres += (tamanhoBloco - bytesUsed); // fragmentação interna
        cout << endl;
    }
    cout << "---------------------------------------------------------" << endl;
    cout << "Total de bytes livres no disco: " << totalBytesLivres << " bytes" << endl;
}

// criar arquivo para cada método de alocação
void criarArquivoContiguo(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    // solicita nome e tamanho do arquivo
    string fileName;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;

    // verifica se o arquivo já existe    
    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;

    // calcula o número de blocos necessários (arredondando para cima)
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;
    
    // verifica se o arquivo cabe no disco
    if (tamanhoBlocos > static_cast<int>(disk.size())) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    bool allocated = false;

    // busca por espaço contíguo livre no disco
    for (int i = 0; i <= static_cast<int>(disk.size()) - tamanhoBlocos; ++i) {
        bool canAllocate = true;
        
        // verifica se os blocos necessários estão livres
        for (int j = 0; j < tamanhoBlocos; ++j) {
            if (disk[i + j] != -1) {
                canAllocate = false;
                break;
            }
        }
        
        if (canAllocate) {
            // cria e inicializa o arquivo
            File newFile;
            newFile.startBlock = i;
            newFile.size = tamanhoBlocos;
            newFile.name = fileName;
            newFile.color = getFileColor(fileID++); // obtém uma cor para o arquivo
            
            // marca os blocos no disco como ocupados pelo arquivo
            for (int j = 0; j < tamanhoBlocos; ++j) {
                disk[i + j] = i;
            }

            // adiciona o arquivo ao map de arquivos files
            files[fileName] = newFile;
            
            tabelaDiretorio[fileName] = make_tuple(i, tamanhoBlocos); // atualiza a tabela de diretório
            fileSizesBytes[fileName] = tamanhoBytes;
            
            cout << "Arquivo criado com sucesso!" << endl;
            displayContiguo(disk, files); // mostra o disco atualizado
            allocated = true;
            break;
        }
    }
    
    // caso não tenha espaço contíguo suficiente, informa erro
    if (!allocated) {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
    }
}

void criarArquivoEncadeado(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    // solicita nome e tamanho do arquivo
    string fileName;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;
    
    // verifica se o arquivo já existe
    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;

    // calcula o número de blocos necessários (arredondando para cima)
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;

    // verifica se o arquivo cabe no disco
    if (tamanhoBlocos > static_cast<int>(disk.size())) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    // busca blocos livres no disco
    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }
    
    // verifica se há blocos livres suficientes
    if (freeBlocks.size() < static_cast<size_t>(tamanhoBlocos)) {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
        return;
    }

    // embaralha os blocos livres para distribuir aleatoriamente
    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);

    // cria a cadeia encadeada de blocos
    int prevBlock = -1;
    vector<int> dataBlocks;
    for (int i = 0; i < tamanhoBlocos; ++i) {
        int currentBlock = freeBlocks[i];
        dataBlocks.push_back(currentBlock);
        
        if (prevBlock != -1) {
            disk[prevBlock] = currentBlock; // ponteiro para o próximo bloco
        }
        prevBlock = currentBlock;
    }

    disk[prevBlock] = -2; // marca o fim da cadeia/arquivo
    
    string color = getFileColor(fileID++); // obtém uma cor para o arquivo

    // cria e inicializa o arquivo
    File newFile;
    newFile.startBlock = dataBlocks[0];
    newFile.dataBlocks = dataBlocks;
    newFile.size = tamanhoBlocos;
    newFile.name = fileName;
    newFile.color = color;

    tabelaDiretorio[fileName] = make_tuple(dataBlocks[0], tamanhoBlocos); // atualiza a tabela de diretório
    fileSizesBytes[fileName] = tamanhoBytes;
    
    // adiciona o arquivo ao map de arquivos files
    files[fileName] = newFile;

    cout << "Arquivo criado com sucesso!" << endl;
    displayEncadeado(disk, files); // mostra o disco atualizado
}

void criarArquivoIndexado(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    // solicita nome e tamanho do arquivo
    string fileName;
    int tamanhoBytes;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;
    
    // verifica se o arquivo já existe
    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo em bytes: ";
    cin >> tamanhoBytes;
    
    // calcula o número de blocos necessários (arredondando para cima)
    int tamanhoBlocos = (tamanhoBytes + tamanhoBloco - 1) / tamanhoBloco;

    // verifica se o arquivo cabe no disco
    if (tamanhoBlocos > static_cast<int>(disk.size())) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    // verifica o bloco índice (suporta no máximo 8 blocos de dados/endereços)
    if (tamanhoBlocos > 8) {
        cout << "Erro: O bloco de índice só pode armazenar até 8 endereços de blocos de dados!" << endl;
        return;
    }

    // busca blocos livres no disco
    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }

    // verifica se há blocos livres suficientes, incluindo o bloco índice
    if (freeBlocks.size() < static_cast<size_t>(tamanhoBlocos + 1)) {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
        return;
    }

    // embaralha os blocos livres para distribuir aleatoriamente (blocos podem estar espalhados pelo disco)
    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);
    
    // seleciona um bloco para ser o bloco índice
    int indexBlock = freeBlocks.back();
    freeBlocks.pop_back();
    
    // cria e inicializa o arquivo
    File newFile;
    newFile.indexBlock = indexBlock;
    newFile.name = fileName;
    newFile.size = tamanhoBlocos;
    
    newFile.color = getFileColor(fileID++); // obtém uma cor para o arquivo
    
    // atribui os blocos de dados e aponta para o bloco índice
    for (int i = 0; i < tamanhoBlocos; ++i) {
        newFile.dataBlocks.push_back(freeBlocks[i]);
        disk[freeBlocks[i]] = indexBlock; // cada bloco de dados aponta para o bloco índice
    }

    disk[indexBlock] = -2; // marca o fim do bloco índice

    // adiciona o arquivo ao map de arquivos files
    files[fileName] = newFile;
    tabelaDiretorio[fileName] = make_tuple(indexBlock, tamanhoBlocos); // atualiza a tabela de diretório
    fileSizesBytes[fileName] = tamanhoBytes;

    cout << "Arquivo criado com sucesso!" << endl;
    displayIndexado(disk, files); // mostra o disco atualizado
}

// deletar arquivo para cada método de alocação
void deleteArquivo(vector<int>& disk, unordered_map<string, File>& filesContiguous, unordered_map<string, File>& filesEncadeados, unordered_map<string, File>& filesIndexados) {
    // solicita o nome do arquivo a ser deletado
    string fileName;
    cout << "Digite o nome do arquivo a ser deletado: ";
    cin >> fileName;

    // verifica se o arquivo é do tipo contíguo
    if (filesContiguous.find(fileName) != filesContiguous.end()) {
        File file = filesContiguous[fileName];
        // libera os blocos ocupados pelo arquivo no disco
        for (int i = file.startBlock; i < file.startBlock + file.size; ++i) {
            disk[i] = -1;
        }
        
        // remove o arquivo do map filesContiguos
        filesContiguous.erase(fileName);
        tabelaDiretorio.erase(fileName);

        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } // verifica se o arquivo é do tipo encadeado
      else if (filesEncadeados.find(fileName) != filesEncadeados.end()) {
        File file = filesEncadeados[fileName];
        // libera os blocos ocupados pelo arquivo no disco
        for (int block : file.dataBlocks) {
            disk[block] = -1;
        }
        
        disk[file.startBlock] = -1; 
        
        // remove o arquivo do map filesEncadeados e tabela de diretório
        filesEncadeados.erase(fileName);
        tabelaDiretorio.erase(fileName);

        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } // verifica se o arquivo é do tipo indexado
      else if (filesIndexados.find(fileName) != filesIndexados.end()) {
        File file = filesIndexados[fileName];
        disk[file.indexBlock] = -1;
        // libera os blocos ocupados pelo arquivo no disco
        for (int block : file.dataBlocks) {
            disk[block] = -1;
        }

        // remove o arquivo do map filesIndexados e tabela de diretório
        filesIndexados.erase(fileName);
        tabelaDiretorio.erase(fileName);

        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } // caso o arquivo não seja encontrado em nenhum dos tipos, informa erro
      else {
        cout << "Erro: Arquivo não encontrado!" << endl;
    }
}

// display da tabela de diretório para os arquivos de cada método de alocação
void displayDiretorioContiguo(const unordered_map<string, File>& files) {
    cout << "\nTabela de Diretório - Alocação Contígua:\n";

    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(15) << "Bloco Inicial" << "| "
         << setw(17) << "Tamanho (blocos)" << "| "
         << setw(15) << "Tamanho (bytes)" << "| "
         << setw(12) << "Fragmentação Interna (bytes)" << "\n";

    // linha de separação
    cout << string(20 + 15 + 17 + 15 + 12 + 30, '-') << "\n";
    
    // percorre todos os arquivos para exibir suas informações
    for (const auto &[nome, file] : files) {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);
        
        if (it != fileSizesBytes.end()) {
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

void displayDiretorioEncadeado(const unordered_map<string, File>& files) {
    cout << "\nTabela de Diretório - Alocação Encadeada:\n";

    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(15) << "Bloco Inicial" << "| "
         << setw(17) << "Tamanho (blocos)" << "| "
         << setw(15) << "Tamanho (bytes)" << "| "
         << setw(22) << "Fragmentação Interna (bytes)" << "\n";
        
    // linha de separação
    cout << string(20 + 15 + 17 + 15 + 22 + 4 * 2 + 18, '-') << "\n";
    
    // percorre todos os arquivos para exibir suas informações
    for (const auto &[nome, file] : files) {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);

        if (it != fileSizesBytes.end()) {
            tamanhoBytes = it->second;
        }

        int fragmentacao = (file.size * tamanhoBloco) - tamanhoBytes;
        cout << left << setw(20) << nome << "| "
             << right << setw(15) << file.startBlock << "| "
             << right << setw(17) << file.size << "| "
             << right << setw(15) << tamanhoBytes << "| "
             << right << setw(22) << fragmentacao << "\n";
        
        // exibe a cadeia de blocos do arquivo
        cout << "Blocos Encadeados: [";
        for (size_t i = 0; i < file.dataBlocks.size(); ++i) {
            cout << file.dataBlocks[i];
            
            if (i < file.dataBlocks.size() - 1)
                cout << " → ";
        }
        cout << "]\n";
    }
}

void displayDiretorioIndexado(const unordered_map<string, File>& files) {
    cout << "\nTabela de Diretório - Alocação Indexada:\n";
    
    cout << left
         << setw(20) << "Arquivo" << "| "
         << setw(16) << "Bloco Índice" << "| "
         << setw(19) << "Tamanho (blocos)" << "| "
         << setw(19) << "Tamanho (bytes)" << "| "
         << setw(28) << "Fragmentação Interna (bytes)" << "\n";
    
    // linha de separação
    cout << string(20 + 15 + 19 + 19 + 28 + 12, '-') << "\n";
    
    // percorre todos os arquivos para exibir suas informações
    for (const auto &[nome, file] : files) {
        int tamanhoBytes = 0;
        auto it = fileSizesBytes.find(nome);
        
        if (it != fileSizesBytes.end()) {
            tamanhoBytes = it->second;
        }

        int fragmentacao = (file.size * tamanhoBloco) - tamanhoBytes;
        cout << left << setw(20) << nome << "| "
             << right << setw(15) << file.indexBlock << "| "
             << right << setw(19) << file.size << "| "
             << right << setw(19) << tamanhoBytes << "| "
             << right << setw(28) << fragmentacao << "\n";
        
        // exibe os blocos de dados do arquivo
        cout << "Blocos de Dados: [";
        for (size_t i = 0; i < file.dataBlocks.size(); ++i) {
            cout << file.dataBlocks[i];
            if (i < file.dataBlocks.size() - 1)
                cout << ", ";
        }
        cout << "]\n";
    }
}

// estender arquivo para cada método de alocação
void estenderArquivoContiguo(vector<int>& disk, unordered_map<string, File>& filesContiguous, unordered_map<string, int>& fileSizesBytes, int blockSize, unordered_map<string, tuple<int,int>>& tabelaDiretorio) {
    // solicita o nome do arquivo e o número de bytes a serem adicionados
    string fileName;
    int adicionalBytes;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesContiguous.find(fileName) == filesContiguous.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de bytes a serem adicionados: ";
    cin >> adicionalBytes;

    if (adicionalBytes <= 0) {
        cout << "Erro: Valor inválido para extensão." << endl;
        return;
    }

    // referência ao arquivo
    File& file = filesContiguous[fileName];
    int start = file.startBlock;
    int blocosOcupados = file.size;
    int discoTotalBlocos = (int)disk.size();

    // tamanho atual em bytes
    int tamanhoAtualBytes = fileSizesBytes[fileName];
    int espacoLivreUltimoBloco = (blocosOcupados * blockSize) - tamanhoAtualBytes;

    int blocosAdicionais = 0;

    // primeiro tenta preencher o espaço livre do último bloco
    if (adicionalBytes <= espacoLivreUltimoBloco) {
        // cabe inteiro no bloco já existente
        fileSizesBytes[fileName] += adicionalBytes;
        adicionalBytes = 0;
    } else {
        // preenche o que falta do último bloco
        adicionalBytes -= espacoLivreUltimoBloco;
        fileSizesBytes[fileName] += espacoLivreUltimoBloco;

        // calcula blocos adicionais realmente necessários
        blocosAdicionais = (adicionalBytes + blockSize - 1) / blockSize;
    }

    int fimArquivo = start + blocosOcupados;

    // verifica se há espaço contíguo disponível para os blocos adicionais
    bool podeEstender = true;
    if (fimArquivo + blocosAdicionais > discoTotalBlocos) {
        podeEstender = false;
    } else {
        for (int i = fimArquivo; i < fimArquivo + blocosAdicionais; ++i) {
            if (disk[i] != -1) {
                podeEstender = false;
                break;
            }
        }
    }

    // realiza a extensão do arquivo se possível
    if (podeEstender) {
        for (int i = fimArquivo; i < fimArquivo + blocosAdicionais; ++i) {
            disk[i] = start; // marca os novos blocos como ocupados pelo arquivo
        }

        // atualiza tamanho em blocos
        file.size += blocosAdicionais;

        // adiciona o restante dos bytes
        fileSizesBytes[fileName] += adicionalBytes;

        // recalcula fragmentação interna
        file.fragmentacao = (file.size * blockSize) - fileSizesBytes[fileName];

        // atualiza a tabela de diretório
        tabelaDiretorio[fileName] = make_tuple(file.startBlock, file.size);

        cout << "Arquivo estendido com sucesso!" << endl;
        displayContiguo(disk, filesContiguous); // mostra o disco atualizado
    } else {
        cout << "Erro: Não há espaço contíguo disponível para extensão!" << endl;
    }
}

void estenderArquivoEncadeado(vector<int>& disk, unordered_map<string, File>& filesEncadeados, unordered_map<string, int>& fileSizesBytes, int blockSize, unordered_map<string, tuple<int,int>>& tabelaDiretorio) {
    // solicita o nome do arquivo e o número de bytes a serem adicionados
    string fileName;
    int adicionalBytes;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesEncadeados.find(fileName) == filesEncadeados.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de bytes a serem adicionados: ";
    cin >> adicionalBytes;

    if (adicionalBytes <= 0) {
        cout << "Erro: Valor inválido para extensão." << endl;
        return;
    }
    
    // referência ao arquivo
    File& file = filesEncadeados[fileName];
    int bytesAtuais = fileSizesBytes[fileName];
    int restanteBytes = adicionalBytes;

    // calcula espaço livre no último bloco existente
    int bytesNoUltimoBloco = bytesAtuais % blockSize;
    int espacoLivreUltimoBloco = (bytesNoUltimoBloco == 0) ? 0 : blockSize - bytesNoUltimoBloco;

    // aproveita o espaço do último bloco parcial
    if (restanteBytes <= espacoLivreUltimoBloco) {
        fileSizesBytes[fileName] += restanteBytes;
        
        // atualiza tamanho total e fragmentação
        file.fragmentacao = (file.dataBlocks.size() * blockSize) - fileSizesBytes[fileName];

        tabelaDiretorio[fileName] = make_tuple(file.startBlock, (int)file.dataBlocks.size()); // atualiza a tabela de diretório
        
        cout << "Arquivo estendido com sucesso!" << endl;
        displayEncadeado(disk, filesEncadeados); // mostra o disco atualizado
        return;
    } else {
        if (espacoLivreUltimoBloco > 0) {
            // preenche o que falta do último bloco
            fileSizesBytes[fileName] += espacoLivreUltimoBloco;
            restanteBytes -= espacoLivreUltimoBloco;
        }
    }

    // calcula quantos blocos adicionais são necessários
    int blocosAdicionais = (restanteBytes + blockSize - 1) / blockSize;

    // busca blocos livres no disco
    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) freeBlocks.push_back(i);
    }

    if ((int)freeBlocks.size() < blocosAdicionais) {
        cout << "Erro: Espaço insuficiente para estender o arquivo!" << endl;
        return;
    }

    // embaralha blocos livres para distribuição aleatória
    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);

    // atualiza ponteiro do último bloco existente somente se há novos blocos
    if (!freeBlocks.empty()) {
        int ultimoBlocoExistente = file.dataBlocks.back();
        disk[ultimoBlocoExistente] = freeBlocks[0];
    }

    // adiciona os novos blocos à cadeia
    for (int i = 0; i < blocosAdicionais; ++i) {
        int atual = freeBlocks[i];
        file.dataBlocks.push_back(atual);

        if (i < blocosAdicionais - 1) {
            disk[atual] = freeBlocks[i + 1];
        } else {
            disk[atual] = -2; // fim da cadeia
        }
    }

    // aualiza tamanho total e fragmentação
    fileSizesBytes[fileName] += restanteBytes;
    file.size = (int)file.dataBlocks.size();
    file.fragmentacao = (file.size * blockSize) - fileSizesBytes[fileName];

    // atualiza a tabela de diretório
    tabelaDiretorio[fileName] = make_tuple(file.startBlock, file.size);

    cout << "Arquivo estendido com sucesso!" << endl;
    displayEncadeado(disk, filesEncadeados); // mostra o disco atualizado
}

void estenderArquivoIndexado(vector<int>& disk, unordered_map<string, File>& filesIndexados, unordered_map<string, int>& fileSizesBytes, int blockSize, unordered_map<string, tuple<int,int>>& tabelaDiretorio) {
    // solicita o nome do arquivo e o número de bytes a serem adicionados
    string fileName;
    int adicionalBytes;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesIndexados.find(fileName) == filesIndexados.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de bytes a serem adicionados: ";
    cin >> adicionalBytes;

    if (adicionalBytes <= 0) {
        cout << "Erro: Valor inválido para extensão." << endl;
        return;
    }

    // referência ao arquivo
    File& file = filesIndexados[fileName];
    int bytesAtuais = fileSizesBytes[fileName];
    int bytesNoUltimoBloco = bytesAtuais % blockSize;
    int restanteBytes = adicionalBytes;

    // aproveita espaço livre no último bloco, se houver
    if (bytesNoUltimoBloco > 0) {
        int espacoLivre = blockSize - bytesNoUltimoBloco;
        int bytesParaAdicionar = min(restanteBytes, espacoLivre);
        fileSizesBytes[fileName] += bytesParaAdicionar;
        restanteBytes -= bytesParaAdicionar;
    }

    // calcula quantos blocos inteiros adicionais são necessários
    int blocosAdicionais = (restanteBytes + blockSize - 1) / blockSize;

    // limitando o bloco de índice (cada endereço ocupa 1 byte - máximo 8)
    // verifica se o bloco índice suporta os blocos adicionais
    int maxEntradasIndice = 8;
    int entradasAtuais = file.dataBlocks.size();
    if (entradasAtuais + blocosAdicionais > maxEntradasIndice) {
        cout << "Erro: Não é possível estender, bloco índice cheio!" << endl;
        cout << "Entradas atuais: " << entradasAtuais << ", blocos a adicionar: " << blocosAdicionais << ", limite máximo: " << maxEntradasIndice << endl;
        return;
    }

    if (blocosAdicionais > 0) {
        // procura por blocos livres no disco
        vector<int> freeBlocks;
        for (size_t i = 0; i < disk.size(); ++i) {
            if (disk[i] == -1) freeBlocks.push_back(i);
        }

        if ((int)freeBlocks.size() < blocosAdicionais) {
            cout << "Erro: Espaço insuficiente para estender o arquivo!" << endl;
            return;
        }

        // embaralha os blocos livres aleatoriamente
        random_device rd;
        mt19937 g(rd());
        shuffle(freeBlocks.begin(), freeBlocks.end(), g);

        // adiciona os novos blocos e aponta para o bloco índice
        for (int i = 0; i < blocosAdicionais; ++i) {
            int bloco = freeBlocks[i];
            disk[bloco] = file.indexBlock;
            file.dataBlocks.push_back(bloco);
        }

        fileSizesBytes[fileName] += restanteBytes;
    }

    // atualiza tamanho total e fragmentação
    file.size = (int)file.dataBlocks.size();
    file.fragmentacao = (file.size * blockSize) - fileSizesBytes[fileName];
    // atualiza a tabela de diretório
    tabelaDiretorio[fileName] = make_tuple(file.indexBlock, file.size);

    cout << "Arquivo estendido com sucesso!" << endl;
    displayIndexado(disk, filesIndexados); // mostra o disco atualizado
}

// simular leitura dos arquivos para cada método de alocação
void simularLeituraContiguo(const unordered_map<string, File>& filesContiguous, const unordered_map<string, int>& fileSizesBytes, int blockSize, int t_sequencial = 1, int t_aleatorio = 6){
    // solicita o nome do arquivo a ser lido
    string fileName;
    cout << "Digite o nome do arquivo para simular a leitura: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesContiguous.find(fileName) == filesContiguous.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    const File& file = filesContiguous.at(fileName);

    // monta o vetor de blocos do arquivo
    vector<int> blocosArquivo;
    for (int i = 0; i < file.size; ++i) {
        blocosArquivo.push_back(file.startBlock + i);
    }

    if (blocosArquivo.empty()) {
        cout << "Erro: Arquivo vazio ou inválido!" << endl;
        return;
    }

    int fragmentacao = (file.size * blockSize) - fileSizesBytes.at(fileName);

    // tempo sequencial: percorre todos os blocos em sequência (1 passo por bloco)
    int passosSequenciais = (int)blocosArquivo.size();
    int tempoSequencial = (int)blocosArquivo.size() * t_sequencial;

    cout << "\nSimulação de leitura do arquivo '" << fileName << "' (Contígua):\n";
    cout << "Fragmentação interna: " << fragmentacao << " bytes\n";
    cout << "Leitura sequencial: " << passosSequenciais << " passos | " << tempoSequencial << " ms\n";
    
    // imprime a ordem sequencial dos blocos
    cout << "Ordem sequencial dos blocos: [";
    for (size_t i = 0; i < blocosArquivo.size(); ++i) {
        cout << blocosArquivo[i];
        if (i < blocosArquivo.size() - 1) 
            cout << ", ";
    }

    cout << "]\n";

    // acesso aleatório: solicita o índice do bloco a ser acessado aleatoriamente
    int indiceDesejado;
    cout << "Digite o índice do bloco que deseja acessar (0 = primeiro bloco): ";
    cin >> indiceDesejado;

    if (indiceDesejado < 0 || indiceDesejado >= static_cast<int>(blocosArquivo.size())) {
        cout << "Índice inválido. Digite um valor entre 0 e " << static_cast<int>(blocosArquivo.size()) - 1 << ".\n";
    } else {
        int blocoReal = blocosArquivo[indiceDesejado];
        int passosAleatorios = 1; // acesso direto
        int tempoAleatorio = t_aleatorio;

        cout << "Acesso aleatório ao bloco " << blocoReal << ": " << passosAleatorios << " passo | " << tempoAleatorio << " ms\n";
    }
}

void simularLeituraEncadeado(const vector<int>& disk, const unordered_map<string, File>& filesEncadeados, const unordered_map<string, int>& fileSizesBytes, int blockSize, int t_sequencial = 1, int t_aleatorio = 5) {
    // solicita o nome do arquivo a ser lido
    string fileName;
    cout << "Digite o nome do arquivo para simular a leitura: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesEncadeados.find(fileName) == filesEncadeados.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    const File& file = filesEncadeados.at(fileName);

    // monta o vetor de blocos seguindo os ponteiros
    vector<int> blocosArquivo;
    int blocoAtual = file.dataBlocks.empty() ? -1 : file.dataBlocks.front();

    while (blocoAtual != -1 && blocoAtual != -2) { //fim da cadeia
        blocosArquivo.push_back(blocoAtual);
        blocoAtual = disk[blocoAtual]; // próximo bloco
    }

    if (blocosArquivo.empty()) {
        cout << "Erro: Arquivo vazio ou inválido!" << endl;
        return;
    }

    int fragmentacao = (file.size * blockSize) - fileSizesBytes.at(fileName);

     // custo sequencial: blocos + saltos de ponteiro
    int passosSequenciais = (int)blocosArquivo.size() * 2 - 1; 
    int tempoSequencial = (int)blocosArquivo.size() * t_sequencial;

    cout << "\nSimulação de leitura do arquivo '" << fileName << "' (Encadeada):\n";
    cout << "Fragmentação interna: " << fragmentacao << " bytes\n";
    cout << "Leitura sequencial: " << passosSequenciais << " passos | " << tempoSequencial << " ms\n";

    // imprime ordem sequencial dos blocos
    cout << "Ordem sequencial dos blocos: [";
    for (size_t i = 0; i < blocosArquivo.size(); ++i) {
        cout << blocosArquivo[i];
        if (i < blocosArquivo.size() - 1) cout << ", ";
    }

    cout << "]\n";

    // solicita o índice do bloco a ser acessado aleatoriamente
    int indiceDesejado;
    cout << "Digite o índice do bloco que deseja acessar (0 = primeiro bloco): ";
    cin >> indiceDesejado;

    if (indiceDesejado < 0 || indiceDesejado >= static_cast<int>(blocosArquivo.size())) {
        cout << "Índice inválido. Digite um valor entre 0 e " << static_cast<int>(blocosArquivo.size()) - 1 << ".\n";
    } else {
        // tempo de acesso aleatório: depende da posição (precisa percorrer a cadeia até o bloco desejado)
        int blocoReal = blocosArquivo[indiceDesejado];
        int passosAleatorios = (indiceDesejado + 1) * 2 - 1;
        int tempoAleatorio = (indiceDesejado + 1) * t_aleatorio;

        cout << "Acesso aleatório ao bloco " << blocoReal << ": " << passosAleatorios << " passos | " << tempoAleatorio << " ms\n";

        // percurso da cadeia encadeada até o bloco desejado
        cout << "Percurso até o bloco desejado: ";
        for (int i = 0; i <= indiceDesejado; ++i) {
            cout << blocosArquivo[i];
            
            if (i < indiceDesejado) 
                cout << " -> ";
        }

        cout << "\n";
    }
}

void simularLeituraIndexado(const unordered_map<string, File>& filesIndexados, const unordered_map<string, int>& fileSizesBytes, int blockSize, int t_sequencial = 1, int t_aleatorio = 5, int t_indice = 5) {
    // solicita o nome do arquivo a ser lido
    string fileName;
    cout << "Digite o nome do arquivo para simular a leitura: ";
    cin >> fileName;

    // verifica se o arquivo existe
    if (filesIndexados.find(fileName) == filesIndexados.end()) {
        cout << "Arquivo não encontrado." << endl;
        return;
    }

    const File& file = filesIndexados.at(fileName);
    const vector<int>& blocosArquivo = file.dataBlocks;

    if (blocosArquivo.empty()) {
        cout << "Erro: Arquivo vazio ou inválido!" << endl;
        return;
    }

    int fragmentacao = (file.size * blockSize) - fileSizesBytes.at(fileName);

    // leitura sequencial: ler bloco de índice + percorrer todos os blocos do arquivo
    int passosSequenciais = 1 + (int)blocosArquivo.size();
    int tempoSequencial = t_indice + (int)blocosArquivo.size() * t_sequencial;

    cout << "\nSimulação de leitura do arquivo '" << fileName << "' (Indexada):\n";
    cout << "Fragmentação interna: " << fragmentacao << " bytes\n";
    cout << "Leitura sequencial: " << passosSequenciais << " passos | " << tempoSequencial << " ms\n";

    // imprime a ordem sequencial dos blocos
    cout << "Ordem sequencial dos blocos: [";
    for (size_t i = 0; i < blocosArquivo.size(); ++i) {
        cout << blocosArquivo[i];
        if (i < blocosArquivo.size() - 1) cout << ", ";
    }

    cout << "]\n";

    // acesso aleatório: índice + bloco
    int indiceDesejado;
    cout << "Digite o índice do bloco que deseja acessar (0 = primeiro bloco): ";
    cin >> indiceDesejado;

    if (indiceDesejado < 0 || indiceDesejado >= static_cast<int>(blocosArquivo.size())) {
        cout << "Índice inválido. Digite um valor entre 0 e " << static_cast<int>(blocosArquivo.size()) - 1 << ".\n";
    } else {
        int blocoReal = blocosArquivo[indiceDesejado];
        int tempoBloco = t_indice + t_aleatorio;

        cout << "Tempo para acessar o bloco " << blocoReal << " aleatoriamente: " << tempoBloco << " ms\n";

        // percurso do bloco de índice até o bloco desejado
        cout << "Percurso até o bloco desejado: bloco índice -> " << blocoReal << "\n";
    }
}

// menu principal do simulador com as opções de executar operações em arquivos, permitindo escolher o método de alocação e tamanho do disco
int main() {
    int diskSizeBytes;
    
    do {
        cout << "===== SIMULADOR DE ALOCAÇÃO DE ARQUIVOS =====\n";
        cout << "  Métodos: Contígua | Encadeada | Indexada\n";
        cout << "   Simulação com blocos lógicos de 8 bytes\n";
        cout << "Determine o tamanho do disco em bytes (mínimo 16 (2 blocos) | máximo 1024 bytes (128 blocos)): ";
        cin >> diskSizeBytes;

        if (cin.fail()) { // se não for número
            cin.clear(); // limpa o estado de erro
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // descarta input inválido
            cout << "Entrada inválida! Por favor, digite um número.\n";
            diskSizeBytes = 0; // força a repetição do loop
            continue; 
    }
        // valida o tamanho do disco
        if (diskSizeBytes < 16 || diskSizeBytes > 1024) {
            cout << "Tamanho inválido! Digite novamente, mínimo 16 e máximo 1024.\n";
        }
    } while (diskSizeBytes < 16 || diskSizeBytes > 1024);
    
    // calcula o número de blocos do disco (tamanho do bloco fixo de 8 bytes)
    int diskSizeBlocks = (diskSizeBytes + tamanhoBloco - 1) / tamanhoBloco;
    
    cout << "Disco de " << diskSizeBytes << " bytes criado com " << diskSizeBlocks << "blocos de " << tamanhoBloco << " bytes.\n ";
    
    // inicializa o disco com -1 (bloco livre)
    vector<int> disk(diskSizeBlocks, -1);
    
    cout << "Estado inicial do disco:" << endl;
    for (int i = 0; i < diskSizeBlocks; ++i) {
        cout << "[" << i << "] ░" << endl;
    }

    // contador para gerar cores únicas para os arquivos
    int fileID = 0;

    // maps para armazenar os arquivos de cada método de alocação
    unordered_map<string, File> filesContiguous;
    unordered_map<string, File> filesEncadeados;
    unordered_map<string, File> filesIndexados;

    int tipoAlocacao;
    while (true){
        cout << "Escolha o tipo de alocação:\n1. Contígua\n2. Encadeada\n3. Indexada\n";
        cin >> tipoAlocacao;
    
        if (cin.fail()) { // entrada não numérica
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Entrada inválida! Digite um número entre 1 e 3.\n";
            continue;
        }

        if (tipoAlocacao < 1 || tipoAlocacao > 3) {
            cout << "Opção inválida! Digite 1, 2 ou 3.\n";
            continue;
        }
            break;
        }
    
    while (true) {
        cout << "\nAgora, selecione uma das opções:\n";
        cout << "1. Criar arquivo\n";
        cout << "2. Deletar arquivo\n";
        cout << "3. Mostrar disco\n";
        cout << "4. Mostrar tabela de diretório\n";
        cout << "5. Estender arquivo\n";
        cout << "6. Simular leitura do arquivo (sequencial vs aleatória)\n";
        cout << "7. Encerrar o programa\n";
        int opcao;
        cin >> opcao;

        switch (opcao) {
            case 1:
                if (tipoAlocacao == 1) {
                    criarArquivoContiguo(disk, filesContiguous, fileID);
                } else if (tipoAlocacao == 2) {
                    criarArquivoEncadeado(disk, filesEncadeados, fileID);
                } else if (tipoAlocacao == 3) {
                    criarArquivoIndexado(disk, filesIndexados, fileID);
                } break;
            case 2:
                deleteArquivo(disk, filesContiguous, filesEncadeados, filesIndexados);
                break;
            case 3:
                if (tipoAlocacao == 1) {
                    displayContiguo(disk, filesContiguous);
                } else if (tipoAlocacao == 2) {
                    displayEncadeado(disk, filesEncadeados);
                } else if (tipoAlocacao == 3) {
                    displayIndexado(disk, filesIndexados);
                } break;
            case 4:
                if (tipoAlocacao == 1) {
                    displayDiretorioContiguo(filesContiguous);
                } else if (tipoAlocacao == 2) {
                    displayDiretorioEncadeado(filesEncadeados);
                } else if (tipoAlocacao == 3) {
                    displayDiretorioIndexado(filesIndexados);
                } break;
            case 5:
                if (tipoAlocacao == 1) {
                    estenderArquivoContiguo(disk, filesContiguous, fileSizesBytes, tamanhoBloco, tabelaDiretorio);
                } else if (tipoAlocacao == 2) {
                    estenderArquivoEncadeado(disk, filesEncadeados, fileSizesBytes, tamanhoBloco, tabelaDiretorio); 
                } else if (tipoAlocacao == 3) {
                    estenderArquivoIndexado(disk, filesIndexados, fileSizesBytes, tamanhoBloco, tabelaDiretorio);
                } break;
            case 6: 
                if (tipoAlocacao == 1) {
                    simularLeituraContiguo(filesContiguous, fileSizesBytes, tamanhoBloco);
                } else if (tipoAlocacao == 2) {
                    simularLeituraEncadeado(disk, filesEncadeados, fileSizesBytes, tamanhoBloco);
                } else if (tipoAlocacao == 3) {
                    simularLeituraIndexado(filesIndexados, fileSizesBytes, tamanhoBloco);
                } break;
            case 7:
                cout << "Encerrando o programa..." << endl;
                return 0;
            default:
                cout << "Opção inválida! Digite um número entre 1 e 7.!\n" << endl;
        }
    }
    
    return 0;
}