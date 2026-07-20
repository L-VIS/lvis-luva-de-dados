/**
 * @file giro_pulso.ino
 * @brief Medição do giro máximo do punho nos eixos Roll, Pitch e Yaw (graus)
 * 
 * Utiliza integração da velocidade angular do giroscópio do MPU6050 para
 * medir o ângulo máximo de rotação durante movimentos até o limite anatômico.
 * 
 * @author L-VIS / UnB
 * @date Junho/2026
 * 
 * @section Saída Serial
 * Exibe tabela com 10 repetições por eixo e a média/desvio padrão.
 * 
 * @section Pinagem
 * - MPU6050 conectado via I2C (SDA, SCL)
 */

#include <MPU6050_tockn.h>
#include <Wire.h>

// ========================================================================
//  CONFIGURAÇÕES
// ========================================================================

#define NUM_REPETICOES 10            // 10 repetições por eixo
#define AMOSTRAS_POR_MEDICAO 100     // ~2 segundos de amostragem
#define TAXA_AMOSTRAGEM_MS 50        // 50 ms entre amostras (20 Hz)

// ========================================================================
//  OBJETOS
// ========================================================================

MPU6050 mpu6050(Wire);

// ========================================================================
//  PROTÓTIPOS
// ========================================================================

float integrarVelocidadeAngular(float velocidades[], int n, float dt);
void medirGiro(char eixo);

// ========================================================================
//  SETUP
// ========================================================================

void setup() {
  Serial.begin(4800);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  
  Serial.println("\n=== MEDIÇÃO DE GIRO MÁXIMO DO PULSO (graus) ===");
  Serial.println("Instruções:");
  Serial.println("1. Posicione o punho em repouso (palma para baixo).");
  Serial.println("2. Gire o punho no eixo indicado até o LIMITE ANATÔMICO.");
  Serial.println("3. Mantenha a posição por 1 segundo e retorne.");
  Serial.println("4. O código registrará 10 repetições para cada eixo.");
  Serial.println("\nPressione qualquer tecla no monitor serial para iniciar...");
  while (!Serial.available()) { delay(100); }
  Serial.read();
  
  // Executa a medição para cada eixo
  medirGiro('X');  // Roll
  medirGiro('Y');  // Pitch
  medirGiro('Z');  // Yaw
  
  Serial.println("\n=== FIM DA MEDIÇÃO ===");
}

// ========================================================================
//  LOOP
// ========================================================================

void loop() {
  delay(1000);
}

// ========================================================================
//  FUNÇÕES
// ========================================================================

/**
 * @brief Mede o giro máximo para um eixo específico
 * @param eixo 'X' (roll), 'Y' (pitch) ou 'Z' (yaw)
 */
void medirGiro(char eixo) {
  float dados[NUM_REPETICOES];
  
  String nomeEixo;
  String descricaoMovimento;
  switch (eixo) {
    case 'X': nomeEixo = "Roll"; descricaoMovimento = "rotacionar o punho para a DIREITA (pronosupinação)"; break;
    case 'Y': nomeEixo = "Pitch"; descricaoMovimento = "flexionar o punho para CIMA (extensão)"; break;
    case 'Z': nomeEixo = "Yaw"; descricaoMovimento = "desviar o punho para a ESQUERDA (desvio ulnar)"; break;
  }
  
  Serial.print("\n=== EIXO ");
  Serial.print(eixo);
  Serial.print(" (");
  Serial.print(nomeEixo);
  Serial.println(") ===");
  
  for (int rep = 0; rep < NUM_REPETICOES; rep++) {
    Serial.print("\nRepetição ");
    Serial.print(rep + 1);
    Serial.print(": ");
    Serial.print(descricaoMovimento);
    Serial.println(" até o limite máximo.");
    delay(1000);
    
    // Coleta dados do giroscópio durante o movimento
    float velocidades[AMOSTRAS_POR_MEDICAO];
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      mpu6050.update();
      float gyro = 0;
      switch (eixo) {
        case 'X': gyro = mpu6050.getGyroX(); break;
        case 'Y': gyro = mpu6050.getGyroY(); break;
        case 'Z': gyro = mpu6050.getGyroZ(); break;
      }
      velocidades[i] = gyro;
      delay(TAXA_AMOSTRAGEM_MS);
    }
    
    // Subtrai o offset (drift) usando a média das primeiras 10 amostras
    float offset = 0;
    for (int i = 0; i < 10; i++) offset += velocidades[i];
    offset /= 10;
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      velocidades[i] -= offset;
    }
    
    // Integração da velocidade angular para obter o ângulo total
    float dt = TAXA_AMOSTRAGEM_MS / 1000.0;
    float angulo = integrarVelocidadeAngular(velocidades, AMOSTRAS_POR_MEDICAO, dt);
    
    dados[rep] = angulo;
    
    Serial.print("  Ângulo máximo: ");
    Serial.print(dados[rep], 1);
    Serial.println("°");
  }
  
  // ========================================================================
  //  EXIBIÇÃO DOS RESULTADOS PARA ESTE EIXO
  // ========================================================================
  Serial.println("\n--- RESULTADOS ---");
  
  // Média e desvio padrão
  float soma = 0;
  for (int i = 0; i < NUM_REPETICOES; i++) soma += dados[i];
  float media = soma / NUM_REPETICOES;
  
  float variancia = 0;
  for (int i = 0; i < NUM_REPETICOES; i++) {
    variancia += pow(dados[i] - media, 2);
  }
  float desvio = sqrt(variancia / (NUM_REPETICOES - 1));
  
  Serial.print(nomeEixo);
  Serial.print(": média ");
  Serial.print(media, 1);
  Serial.print("° ± ");
  Serial.print(desvio, 1);
  Serial.println("°");
  
  // Tabela com todas as repetições
  Serial.println("\nTabela de valores individuais:");
  for (int i = 0; i < NUM_REPETICOES; i++) {
    Serial.print("Rep ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(dados[i], 1);
    Serial.println("°");
  }
}

/**
 * @brief Integra a velocidade angular para obter o ângulo total (graus)
 * @param velocidades Array com velocidades angulares (°/s)
 * @param n Número de amostras
 * @param dt Intervalo de tempo entre amostras (segundos)
 * @return Ângulo total (graus)
 */
float integrarVelocidadeAngular(float velocidades[], int n, float dt) {
  float angulo = 0;
  for (int i = 1; i < n; i++) {
    // Método do trapézio para integrar
    angulo += (velocidades[i] + velocidades[i-1]) / 2.0 * dt;
  }
  return angulo;
}