#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <iomanip>
using namespace std;

struct File {
    int indexBlock;
    int startBlock;
    vector<int> dataBlocks;
    int size;
    string name;
    string color;
};

unordered_map<string, int> fileColors;

string getFileColor(int fileID) {
    static const vector<string> colors = {
        "\033[31m", // vermelho
        "\033[32m", // verde
        "\033[33m", // amarelo
        "\033[34m", // azul
        "\033[35m", // rosa
        "\033[36m", // ciano
        "\033[37m",  // branco
        "\033[90m" // cinza
    };
    return colors[fileID % colors.size()];
}

unordered_map<string, tuple<int, int>> tabelaDiretorio;

void displayTabelaDiretorio() {
    cout << "Tabela de Diretorio:" << endl;
    cout << left << setw(22) << "Nome do Arquivo   |"
         << setw(18) << "Bloco Inicial  | "
         << setw(13) << "Tamanho" << endl;

    cout << "-----------------------------------------------------" << endl;

    for (const auto& entry : tabelaDiretorio) {
        cout << left << setw(25) << entry.first
             << setw(20) << get<0>(entry.second)
             << setw(15) << get<1>(entry.second) << endl;
    }
}

void displayContiguo(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Contígua:" << endl;
    for (int i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            cout << "[" << i << "] " << "░" << "\033[0m" << endl;
        } else {
            cout << "[" << i << "] ";
            bool isFileBlock = false;

            for (const auto& file : files) {
                if (i >= file.second.startBlock && i < file.second.startBlock + file.second.size) {
                    cout << file.second.color << "█" << "\033[0m";

                    if (i == file.second.startBlock) {
                        cout << " → INICIO do " << file.first << "" << endl;
                    } else if (i == file.second.startBlock + file.second.size - 1) {
                        cout << " → FIM do " << file.first << endl;
                    } else {
                        cout << " [" << file.first << "]" << endl;
                    }
                    isFileBlock = true;
                    break;
                }
            }

            if (!isFileBlock) {
                cout << "░" << "\033[0m" << endl;
            }
        }
    }
    cout << "------------------------------------" << endl;
}

void displayEncadeado(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Encadeada:" << endl;

    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            cout << "[" << i << "] ░" << endl;
        } else {
            string color = "\033[0m";
            string fileName;
            bool isStartBlock = false;

            for (const auto& [fileNameKey, file] : files) {
                if (file.startBlock == i) {
                    color = file.color;
                    fileName = file.name;
                    isStartBlock = true;
                    break;
                } else if (find(file.dataBlocks.begin(), file.dataBlocks.end(), i) != file.dataBlocks.end()) {
                    color = file.color;
                    fileName = file.name;
                    break;
                }
            }

            cout << "[" << i << "] " << color << "█\033[0m → ";

            if (isStartBlock) {
                cout << "INICIO → ";
            }

            if (disk[i] == -2) {
                cout << "FIM do " << fileName;
            } else {
                cout << "[" << disk[i] << "]";
            }

            if (isStartBlock) {
                cout << " do " << fileName;
            }
            cout << endl;
        }
    }
    cout << "--------------------------------------" << endl;
}

void displayIndexado(const vector<int>& disk, const unordered_map<string, File>& files) {
    cout << "Memória Indexada:" << endl;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            cout << "[" << i << "] ░" << endl;
        } else {
            for (const auto& file : files) {
                if (file.second.indexBlock == i || find(file.second.dataBlocks.begin(), file.second.dataBlocks.end(), i) != file.second.dataBlocks.end()) {
                    cout << "[" << i << "] " << file.second.color << "█" << "\033[0m";
                    if (file.second.indexBlock == i) {
                        cout << " → BLOCO DE INDICE -> [";
                        for (size_t j = 0; j < file.second.dataBlocks.size(); ++j) {
                            cout << file.second.dataBlocks[j];
                            if (j < file.second.dataBlocks.size() - 1)
                                cout << ", ";
                        }
                        cout << "] do " << file.first;
                    }
                    cout << endl;
                    break;
                }
            }
        }
    }
    cout << "------------------------------------" << endl;
}

void createArquivoContiguo(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    string fileName;
    int size;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;

    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo (em blocos): ";
    cin >> size;

    if (size > disk.size()) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    bool allocated = false;
    for (int i = 0; i < disk.size() - size + 1; ++i) {
        bool canAllocate = true;

        for (int j = 0; j < size; ++j) {
            if (disk[i + j] != -1) {
                canAllocate = false;
                break;
            }
        }

        if (canAllocate) {
            File newFile;
            newFile.startBlock = i;
            newFile.size = size;
            newFile.name = fileName;
            newFile.color = getFileColor(fileID++);
            for (int j = 0; j < size; ++j) {
                disk[i + j] = i;
            }
            files[fileName] = newFile;
            tabelaDiretorio[fileName] = make_tuple(i, size);
            cout << "Arquivo criado com sucesso!" << endl;
            displayContiguo(disk, files);
            allocated = true;
            break;
        }
    }

    if (!allocated) {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
    }
}

void createArquivoEncadeado(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    string fileName;
    int size;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;

    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo (em blocos): ";
    cin >> size;

    if (size > disk.size()) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }

    if (freeBlocks.size() < static_cast<size_t>(size)) {
        cout << "Erro: Espaço insuficiente no disco!" << endl;
        return;
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);

    int prevBlock = -1;
    vector<int> dataBlocks;
    for (int i = 0; i < size; ++i) {
        int currentBlock = freeBlocks[i];
        dataBlocks.push_back(currentBlock);
        if (prevBlock != -1) {
            disk[prevBlock] = currentBlock;
        }
        prevBlock = currentBlock;
    }
    disk[prevBlock] = -2;

    string color = getFileColor(fileID++);
    
    File newFile;
    newFile.startBlock = dataBlocks[0];
    newFile.dataBlocks = dataBlocks;
    newFile.name = fileName;
    newFile.color = color;
    
    tabelaDiretorio[fileName] = make_tuple(dataBlocks[0], size);

    files[fileName] = newFile;

    cout << "Arquivo criado com sucesso!" << endl;
    displayEncadeado(disk, files);
}

void createArquivoIndexado(vector<int>& disk, unordered_map<string, File>& files, int& fileID) {
    string fileName;
    int size;

    cout << "Digite o nome do arquivo: ";
    cin >> fileName;

    if (files.find(fileName) != files.end()) {
        cout << "Erro: Arquivo já existe!" << endl;
        return;
    }

    cout << "Digite o tamanho do arquivo (em blocos): ";
    cin >> size;

    if (size > disk.size()) {
        cout << "Erro: Tamanho do arquivo maior que o tamanho do disco!" << endl;
        return;
    }

    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }

    if (freeBlocks.size() < static_cast<size_t>(size + 1)) {
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
    newFile.size = size;
    newFile.color = getFileColor(fileID++);
    for (int i = 0; i < size; ++i) {
        newFile.dataBlocks.push_back(freeBlocks[i]);
        disk[freeBlocks[i]] = indexBlock;
    }
    disk[indexBlock] = -2;

    files[fileName] = newFile;
    tabelaDiretorio[fileName] = make_tuple(indexBlock, size);
    cout << "Arquivo criado com sucesso!" << endl;
    displayIndexado(disk, files);
}

void deleteArquivo(vector<int>& disk, unordered_map<string, File>& filesIndexados, unordered_map<string, File>& filesEncadeados, unordered_map<string, File>& filesContiguous) {
    string fileName;
    cout << "Digite o nome do arquivo a ser deletado: ";
    cin >> fileName;

    if (filesContiguous.find(fileName) != filesContiguous.end()) {
        File file = filesContiguous[fileName];
        for (int i = file.startBlock; i < file.startBlock + file.size; ++i) {
            disk[i] = -1;
        }
        filesContiguous.erase(fileName);
        tabelaDiretorio.erase(fileName);  
        
        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } else if (filesEncadeados.find(fileName) != filesEncadeados.end()) {
        File file = filesEncadeados[fileName];
        for (int block : file.dataBlocks) {
            disk[block] = -1;
        }
        disk[file.startBlock] = -1;
        filesEncadeados.erase(fileName);
        tabelaDiretorio.erase(fileName);  

        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } else if (filesIndexados.find(fileName) != filesIndexados.end()) {
        File file = filesIndexados[fileName];
        disk[file.indexBlock] = -1;
        for (int block : file.dataBlocks) {
            disk[block] = -1;
        }
        filesIndexados.erase(fileName);
        tabelaDiretorio.erase(fileName);  

        cout << "Arquivo " << fileName << " deletado com sucesso!" << endl;
    } else {
        cout << "Erro: Arquivo não encontrado!" << endl;
    }
}

void estenderArquivoContiguo(vector<int>& disk, unordered_map<string, File>& filesContiguous) {
    string fileName;
    int adicional;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    if (filesContiguous.find(fileName) == filesContiguous.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de blocos a serem adicionados: ";
    cin >> adicional;

    File& file = filesContiguous[fileName];
    int start = file.startBlock;
    int end = start + file.size;

    bool podeEstender = true;
    if (end + adicional > disk.size()) {
        podeEstender = false;
    } else {
        for (int i = end; i < end + adicional; ++i) {
            if (disk[i] != -1) {
                podeEstender = false;
                break;
            }
        }
    }

    if (podeEstender) {
        for (int i = end; i < end + adicional; ++i) {
            disk[i] = start;
        }
        file.size += adicional;
        tabelaDiretorio[fileName] = make_tuple(file.startBlock, file.size);
        cout << "Arquivo estendido com sucesso!" << endl;
        displayContiguo(disk, filesContiguous);
    } else {
        cout << "Erro: Não há espaço contíguo disponível para extensão!" << endl;
    }
}

void estenderArquivoEncadeado(vector<int>& disk, unordered_map<string, File>& filesEncadeados) {
    string fileName;
    int adicional;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    if (filesEncadeados.find(fileName) == filesEncadeados.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de blocos a serem adicionados: ";
    cin >> adicional;

    File& file = filesEncadeados[fileName];

    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }

    if (freeBlocks.size() < static_cast<size_t>(adicional)) {
        cout << "Erro: Espaço insuficiente!" << endl;
        return;
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);

    int lastBlock = file.dataBlocks.back();
    disk[lastBlock] = freeBlocks[0];

    for (int i = 0; i < adicional; ++i) {
        int current = freeBlocks[i];
        file.dataBlocks.push_back(current);
        if (i < adicional - 1) {
            disk[current] = freeBlocks[i + 1];
        } else {
            disk[current] = -2; // fim
        }
    }

    tabelaDiretorio[fileName] = make_tuple(file.startBlock, file.dataBlocks.size());
    cout << "Arquivo estendido com sucesso!" << endl;
    displayEncadeado(disk, filesEncadeados);
}

void estenderArquivoIndexado(vector<int>& disk, unordered_map<string, File>& filesIndexados) {
    string fileName;
    int adicional;

    cout << "Digite o nome do arquivo a ser estendido: ";
    cin >> fileName;

    if (filesIndexados.find(fileName) == filesIndexados.end()) {
        cout << "Erro: Arquivo não encontrado!" << endl;
        return;
    }

    cout << "Digite o número de blocos a serem adicionados: ";
    cin >> adicional;

    File& file = filesIndexados[fileName];

    vector<int> freeBlocks;
    for (size_t i = 0; i < disk.size(); ++i) {
        if (disk[i] == -1) {
            freeBlocks.push_back(i);
        }
    }

    if (freeBlocks.size() < static_cast<size_t>(adicional)) {
        cout << "Erro: Espaço insuficiente!" << endl;
        return;
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(freeBlocks.begin(), freeBlocks.end(), g);

    for (int i = 0; i < adicional; ++i) {
        int block = freeBlocks[i];
        disk[block] = file.indexBlock;
        file.dataBlocks.push_back(block);
    }

    file.size = file.dataBlocks.size();
    tabelaDiretorio[fileName] = make_tuple(file.indexBlock, file.size);
    cout << "Arquivo estendido com sucesso!" << endl;
    displayIndexado(disk, filesIndexados);
}

int main() {

    int diskSize;
    do {
        cout << "Determine o tamanho do disco (mínimo 5 e máximo 50): ";
        cin >> diskSize;
    } while (diskSize < 5 || diskSize > 50);

    cout << "Estado inicial do disco:" << endl;
    vector<int> disk(diskSize, -1);

    for (int i = 0; i < diskSize; ++i) {
        cout << "[" << i << "] ░" << endl;
    }
    int fileID = 0;

    unordered_map<string, File> filesContiguous;
    unordered_map<string, File> filesEncadeados;
    unordered_map<string, File> filesIndexados;

    int tipoAlocacao;
    cout << "Escolha o tipo de alocação:\n1. Contígua\n2. Encadeada\n3. Indexada\n";
    cin >> tipoAlocacao;

     while (true) {
        cout <<  "Agora, selecione uma das opções:\n";
        cout << "1. Criar arquivo\n";
        cout << "2. Deletar arquivo\n";
        cout << "3. Mostrar disco\n";
        cout << "4. Mostrar tabela de diretório\n";
        cout << "5. Estender arquivo\n";
        cout << "6. Sair do programa\n";
        int opcao;
        cin >> opcao;

        switch (opcao) {
            case 1:
                if (tipoAlocacao == 1) {
                    createArquivoContiguo(disk, filesContiguous, fileID);
                } else if (tipoAlocacao == 2) {
                    createArquivoEncadeado(disk, filesEncadeados, fileID);
                } else if (tipoAlocacao == 3) {
                    createArquivoIndexado(disk, filesIndexados, fileID);
                } 
                break;
            case 2:
                deleteArquivo(disk, filesIndexados, filesEncadeados, filesContiguous);
                break;
            case 3:
                if (tipoAlocacao == 1) {
                    displayContiguo(disk, filesContiguous);
                 } else if (tipoAlocacao == 2) {
                    displayEncadeado(disk, filesEncadeados);
                } else if (tipoAlocacao == 3) {
                    displayIndexado(disk, filesIndexados);
                }
                break;
            case 4:
              displayTabelaDiretorio();
                break;
            case 5:
                if (tipoAlocacao == 1) {
                    estenderArquivoContiguo(disk, filesContiguous);
                } else if (tipoAlocacao == 2) {
                    estenderArquivoEncadeado(disk, filesEncadeados);
                } else if (tipoAlocacao == 3) {
                    estenderArquivoIndexado(disk, filesIndexados);
                }
                break;
            case 6:
                cout << "Encerrar programa\n";
                return 0;
            default:
                cout << "Opção inválida! Tente novamente\n";
        }
    }
}