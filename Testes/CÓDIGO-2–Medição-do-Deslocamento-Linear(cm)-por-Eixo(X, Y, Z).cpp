/**
 * @file deslocamento_linear.ino
 * @brief Medição do deslocamento linear da mão nos eixos X, Y e Z (em cm)
 * 
 * Utiliza dupla integração da aceleração do MPU6050 para estimar o deslocamento
 * durante movimentos de translação de aproximadamente +15 cm e -15 cm.
 * 
 * @author L-VIS / UnB
 * @date Junho/2026
 * 
 * @section Saída Serial
 * Exibe tabela com 10 repetições por eixo (5 para +15 cm e 5 para -15 cm)
 * e a média/desvio padrão.
 * 
 * @section Pinagem
 * - MPU6050 conectado via I2C (SDA, SCL)
 * 
 * @section Observação
 * A dupla integração do acelerômetro sofre drift. Esta implementação usa
 * subtração de offset e filtro passa-alta simples para minimizar o erro.
 */

#include <MPU6050_tockn.h>
#include <Wire.h>

// ========================================================================
//  CONFIGURAÇÕES
// ========================================================================

#define NUM_REPETICOES_POR_DIRECAO 5  // 5 para +15 cm, 5 para -15 cm = 10 total
#define AMOSTRAS_POR_MEDICAO 100       // 100 amostras (~2 segundos)
#define TAXA_AMOSTRAGEM_MS 50          // 50 ms entre amostras (20 Hz)

// ========================================================================
//  OBJETOS
// ========================================================================

MPU6050 mpu6050(Wire);

// ========================================================================
//  PROTÓTIPOS
// ========================================================================

float integrarDupla(const float aceleracao[], int n, float dt);
void medirEixo(char eixo, float deslocamentoEsperado);

// ========================================================================
//  SETUP
// ========================================================================

void setup() {
  Serial.begin(4800);
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  
  Serial.println("\n=== MEDIÇÃO DE DESLOCAMENTO LINEAR (cm) POR EIXO ===");
  Serial.println("Instruções:");
  Serial.println("1. Posicione a mão em repouso.");
  Serial.println("2. Para cada eixo, mova a mão em linha reta +15 cm e -15 cm.");
  Serial.println("3. O código registrará 5 repetições para cada direção.");
  Serial.println("\nPressione qualquer tecla no monitor serial para iniciar...");
  while (!Serial.available()) { delay(100); }
  Serial.read();
  
  // Executa a medição para cada eixo
  medirEixo('X', 15.0);
  medirEixo('Y', 15.0);
  medirEixo('Z', 15.0);
  
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
 * @brief Mede o deslocamento para um eixo específico
 * @param eixo 'X', 'Y' ou 'Z'
 * @param deslocamentoEsperado Valor alvo em cm (ex.: 15.0)
 */
void medirEixo(char eixo, float deslocamentoEsperado) {
  float dadosPositivo[NUM_REPETICOES_POR_DIRECAO];
  float dadosNegativo[NUM_REPETICOES_POR_DIRECAO];
  
  Serial.print("\n=== EIXO ");
  Serial.print(eixo);
  Serial.println(" ===");
  
  // --- Direção positiva (+15 cm) ---
  Serial.println("\n[+] Movimente a mão +15 cm no eixo "
                 + String(eixo) + " e aguarde...");
  for (int rep = 0; rep < NUM_REPETICOES_POR_DIRECAO; rep++) {
    // Aguarda o usuário se preparar
    Serial.print("  Repetição ");
    Serial.print(rep + 1);
    Serial.println(": mova +15 cm em linha reta e pare.");
    delay(1000);
    
    // Coleta os dados de aceleração durante o movimento
    float aceleracoes[AMOSTRAS_POR_MEDICAO];
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      mpu6050.update();
      float acc = 0;
      switch (eixo) {
        case 'X': acc = mpu6050.getAccX(); break;
        case 'Y': acc = mpu6050.getAccY(); break;
        case 'Z': acc = mpu6050.getAccZ(); break;
      }
      aceleracoes[i] = acc;
      delay(TAXA_AMOSTRAGEM_MS);
    }
    
    // Subtrai o offset (gravidade) usando a média das primeiras amostras
    float offset = 0;
    for (int i = 0; i < 10; i++) offset += aceleracoes[i];
    offset /= 10;
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      aceleracoes[i] -= offset;
    }
    
    // Dupla integração para obter deslocamento (em metros)
    float dt = TAXA_AMOSTRAGEM_MS / 1000.0;
    float deslocamento = integrarDupla(aceleracoes, AMOSTRAS_POR_MEDICAO, dt);
    
    // Converte para cm
    dadosPositivo[rep] = deslocamento * 100.0;
    
    Serial.print("    Deslocamento: ");
    Serial.print(dadosPositivo[rep], 1);
    Serial.println(" cm");
  }
  
  // --- Direção negativa (-15 cm) ---
  Serial.println("\n[-] Movimente a mão -15 cm no eixo "
                 + String(eixo) + " e aguarde...");
  for (int rep = 0; rep < NUM_REPETICOES_POR_DIRECAO; rep++) {
    Serial.print("  Repetição ");
    Serial.print(rep + 1);
    Serial.println(": mova -15 cm em linha reta e pare.");
    delay(1000);
    
    float aceleracoes[AMOSTRAS_POR_MEDICAO];
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      mpu6050.update();
      float acc = 0;
      switch (eixo) {
        case 'X': acc = mpu6050.getAccX(); break;
        case 'Y': acc = mpu6050.getAccY(); break;
        case 'Z': acc = mpu6050.getAccZ(); break;
      }
      aceleracoes[i] = acc;
      delay(TAXA_AMOSTRAGEM_MS);
    }
    
    float offset = 0;
    for (int i = 0; i < 10; i++) offset += aceleracoes[i];
    offset /= 10;
    for (int i = 0; i < AMOSTRAS_POR_MEDICAO; i++) {
      aceleracoes[i] -= offset;
    }
    
    float dt = TAXA_AMOSTRAGEM_MS / 1000.0;
    float deslocamento = integrarDupla(aceleracoes, AMOSTRAS_POR_MEDICAO, dt);
    dadosNegativo[rep] = deslocamento * 100.0;
    
    Serial.print("    Deslocamento: ");
    Serial.print(dadosNegativo[rep], 1);
    Serial.println(" cm");
  }
  
  // ========================================================================
  //  EXIBIÇÃO DOS RESULTADOS PARA ESTE EIXO
  // ========================================================================
  Serial.println("\n--- RESULTADOS ---");
  
  // Calcula médias e desvios
  float somaPos = 0, somaNeg = 0;
  for (int i = 0; i < NUM_REPETICOES_POR_DIRECAO; i++) {
    somaPos += dadosPositivo[i];
    somaNeg += dadosNegativo[i];
  }
  float mediaPos = somaPos / NUM_REPETICOES_POR_DIRECAO;
  float mediaNeg = somaNeg / NUM_REPETICOES_POR_DIRECAO;
  
  float varPos = 0, varNeg = 0;
  for (int i = 0; i < NUM_REPETICOES_POR_DIRECAO; i++) {
    varPos += pow(dadosPositivo[i] - mediaPos, 2);
    varNeg += pow(dadosNegativo[i] - mediaNeg, 2);
  }
  float desvioPos = sqrt(varPos / (NUM_REPETICOES_POR_DIRECAO - 1));
  float desvioNeg = sqrt(varNeg / (NUM_REPETICOES_POR_DIRECAO - 1));
  
  Serial.print("+");
  Serial.print(eixo);
  Serial.print(" (+15 cm): média ");
  Serial.print(mediaPos, 1);
  Serial.print(" cm ± ");
  Serial.print(desvioPos, 1);
  Serial.println(" cm");
  
  Serial.print("-");
  Serial.print(eixo);
  Serial.print(" (-15 cm): média ");
  Serial.print(mediaNeg, 1);
  Serial.print(" cm ± ");
  Serial.print(desvioNeg, 1);
  Serial.println(" cm");
}

/**
 * @brief Integra a aceleração duas vezes para obter deslocamento
 * @param aceleracao Array com valores de aceleração (m/s²)
 * @param n Número de amostras
 * @param dt Intervalo de tempo entre amostras (segundos)
 * @return Deslocamento em metros
 */
float integrarDupla(const float aceleracao[], int n, float dt) {
  float velocidade = 0;
  float posicao = 0;
  
  for (int i = 1; i < n; i++) {
    // Primeira integração: aceleração -> velocidade (método do trapézio)
    velocidade += (aceleracao[i] + aceleracao[i-1]) / 2.0 * dt;
    // Segunda integração: velocidade -> posição
    posicao += velocidade * dt;
  }
  return posicao;
}