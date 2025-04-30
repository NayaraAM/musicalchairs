#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <random>

// Global variables for synchronization
constexpr int NUM_JOGADORES = 4; 
std::counting_semaphore<NUM_JOGADORES> cadeira_sem(NUM_JOGADORES - 1); // Inicia com n-1 cadeiras, capacidade máxima n
std::condition_variable music_cv;
std::mutex music_mutex;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};

int num_cadeiras = 1; // linha escrita

/*
 * Uso básico de um counting_semaphore em C++:
 * 
 * O std::counting_semaphore é um mecanismo de sincronização que permite controlar o acesso a um recurso compartilhado 
 * com um número máximo de acessos simultâneos. Neste projeto, ele é usado para gerenciar o número de cadeiras disponíveis.
 * Inicializamos o semáforo com n - 1 para representar as cadeiras disponíveis no início do jogo. 
 * Cada jogador que tenta se sentar precisa fazer um acquire(), e o semáforo permite que até n - 1 jogadores 
 * ocupem as cadeiras. Quando todos os assentos estão ocupados, jogadores adicionais ficam bloqueados até que 
 * o coordenador libere o semáforo com release(), sinalizando a eliminação dos jogadores.
 * O método release() também pode ser usado para liberar múltiplas permissões de uma só vez, por exemplo: cadeira_sem.release(3);,
 * o que permite destravar várias threads de uma só vez, como é feito na função liberar_threads_eliminadas().
 *
 * Métodos da classe std::counting_semaphore:
 * 
 * 1. acquire(): Decrementa o contador do semáforo. Bloqueia a thread se o valor for zero.
 *    - Exemplo de uso: cadeira_sem.acquire(); // Jogador tenta ocupar uma cadeira.
 * 
 * 2. release(int n = 1): Incrementa o contador do semáforo em n. Pode liberar múltiplas permissões.
 *    - Exemplo de uso: cadeira_sem.release(2); // Libera 2 permissões simultaneamente.
 */

// Classes
class JogoDasCadeiras { // linha do código original
public:
    JogoDasCadeiras(int num_jogadores) // linha do código original
        : num_jogadores(num_jogadores), cadeiras(num_jogadores - 1) {} // linha do código original

    void iniciar_rodada() { // linha do código original
        // TODO: Inicia uma nova rodada, removendo uma cadeira e ressincronizando o semáforo
        cadeiras--;
        num_cadeiras = 1;
        while (cadeira_sem.try_acquire())
            ;
        cadeira_sem.release(cadeiras);
        musica_parada.store(false);

        if (cadeiras > 0) { //verificar aqui se cadeiras > 1 ou cadeiras > 0
            std::cout << "\nPróxima rodada com " << cadeiras + 1 << " jogadores e " << cadeiras << " cadeiras.\n";
            std::cout << "A música está tocando... 🎵\n\n"<< std::flush;
        }

    }

    void parar_musica() { // linha do código original
        // TODO: Simula o momento em que a música para e notifica os jogadores via variável de condição
        std::unique_lock<std::mutex> lock(music_mutex);
        musica_parada.store(true);
        music_cv.notify_all();
        std::cout << "> A música parou! Os jogadores estão tentando se sentar...\n\n";
        std::cout << "----------------------------------------------------------\n";
    }

    void eliminar_jogador(int jogador_id) { // linha do código original
        // TODO: Elimina um jogador que não conseguiu uma cadeira
        std::cout << "\nJogador P" << jogador_id << " não conseguiu uma cadeira e foi eliminado!\n";
        std::cout << "----------------------------------------------------------\n";
    }

    void exibir_estado() { // linha do código original
        // TODO: Exibe o estado atual das cadeiras e dos jogadores
        std::cout << "Rodada atual com " << cadeiras << " cadeiras disponíveis.\n";
    }

    bool jogo_em_progresso() const {
        return cadeiras > 0;
    }

    //verificar se seria = bool jogo_em_progresso(int jogadores_ativos) const{
       // return jogadores_ativos > 1;

private: // linha do código original
    int num_jogadores; // linha do código original
    int cadeiras; // linha do código original
};

class Jogador { // linha do código original
public: // linha do código original
    Jogador(int id, JogoDasCadeiras& jogo) // linha do código original
        : id(id), jogo(jogo), ativo(true), tentou_rodada(false){}

    
    
void tentar_ocupar_cadeira() { // linha do código original
        // TODO: Tenta ocupar uma cadeira utilizando o semáforo contador quando a música para (aguarda pela variável de condição)
        if (ativo && !tentou_rodada) {
            tentou_rodada = true;
            if (cadeira_sem.try_acquire()) {
                std::cout << "[Cadeira " << num_cadeiras++ << "]: Ocupada por P" << id << "\n";
            } else {
                ativo = false;
                jogo.eliminar_jogador(id);
            }
        }
    }

    void verificar_eliminacao() { // linha do código original
        // TODO: Verifica se foi eliminado após ser destravado do semáforo

        // Já feito no tentar_ocupar_cadeira
    }

    void joga() { // linha do código original
        // TODO: Aguarda a música parar usando a variavel de condicao
        // TODO: Tenta ocupar uma cadeira
        // TODO: Verifica se foi eliminado
        while (ativo && jogo_ativo.load()) {
            std::unique_lock<std::mutex> lock(music_mutex);
            music_cv.wait(lock, [] { return musica_parada.load() || !jogo_ativo.load(); });

            if (!jogo_ativo.load()) break;

            tentar_ocupar_cadeira();
        
        }
    }

    void resetar_rodada() {
        tentou_rodada = false;
    }

    bool esta_ativo() const {
        return ativo;
    }
    
    int get_id() const {
        return id;
    }
private: // linha do código original
    int id; // linha do código original
    JogoDasCadeiras& jogo; // linha do código original
    bool ativo;
    bool tentou_rodada;
};

class Coordenador { // linha do código original
public: // linha do código original
    Coordenador(JogoDasCadeiras& jogo, std::vector<Jogador>& jogadores) // linha do código original
        : jogo(jogo), jogadores(jogadores) {} // linha do código original

    void iniciar_jogo() { // linha do código original
        // TODO: Começa o jogo, dorme por um período aleatório, e então para a música, sinalizando os jogadores
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1000, 3000);

        while (jogo.jogo_em_progresso()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
            jogo.parar_musica();

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            liberar_threads_eliminadas();
            jogo.iniciar_rodada();

            for (auto& jogador : jogadores) {
                jogador.resetar_rodada();
         
            }
        }


        jogo_ativo.store(false);
        music_cv.notify_all();
    }
    int get_vencedor() {
        for (auto& jogador : jogadores) {
            if (jogador.esta_ativo()) {
                return jogador.get_id();
            }
        }
        return -1;
    }
    void liberar_threads_eliminadas() { // linha do código original
        // Libera várias permissões no semáforo para destravar todas as threads que não conseguiram se sentar
        cadeira_sem.release(NUM_JOGADORES - 1);
    }

private: // linha do código original
    JogoDasCadeiras& jogo;// linha do código original
    std::vector<Jogador>& jogadores;
};

// Main function
int main() { // linha do código original
    std::cout << "----------------------------------------------------------\n";
    std::cout << "Bem-vindo ao Jogo das Cadeiras Concorrente!\n";
    std::cout << "----------------------------------------------------------\n";

    std::cout << "\nIniciando rodada com " << NUM_JOGADORES << " jogadores e " << NUM_JOGADORES - 1 << " cadeiras\n";
    std::cout << "A música está tocando... 🎵\n\n";

    JogoDasCadeiras jogo(NUM_JOGADORES); // linha do código original
 // linha do código original
    std::vector<std::thread> jogadores; // linha do código original


    // Criação das threads dos jogadores
    std::vector<Jogador> jogadores_objs; // linha do código original
    for (int i = 1; i <= NUM_JOGADORES; ++i) { // linha do código original
        jogadores_objs.emplace_back(i, jogo); // linha do código original
    }

    for (int i = 0; i < NUM_JOGADORES; ++i) { // linha do código original
        jogadores.emplace_back(&Jogador::joga, &jogadores_objs[i]); // linha do código original
    }
    Coordenador coordenador(jogo, jogadores_objs);
    // Thread do coordenador
    std::thread coordenador_thread(&Coordenador::iniciar_jogo, &coordenador); // linha do código original

    // Esperar pelas threads dos jogadores
    for (auto& t : jogadores) { // linha do código original
        if (t.joinable()) { // linha do código original
            t.join(); // linha do código original
        }
    }

    // Esperar pela thread do coordenador
    if (coordenador_thread.joinable()) { // linha do código original
        coordenador_thread.join(); // linha do código original
    }

    int id = coordenador.get_vencedor();

    if (id != -1) {
        std::cout << "\n🏆 Vencedor: Jogador P" << id << "! Parabéns! 🏆\n";
        std::cout << "----------------------------------------------------------\n";
    }

    std::cout << "Jogo das Cadeiras finalizado.\n" << std::endl; // linha do código original
    return 0;
}
