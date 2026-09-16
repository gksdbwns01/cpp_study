#include <stdio.h>
#include "api.h"
#include "poly.h"
#include "randombytes.h"
#include "fips202.h"

/*************************************************
* Name:        encode_pk
* 
* Description: Serialize the public key as concatenation of the
*              serialization of the polynomial pk and the public seed
*              used to generete the polynomial a.
*
* Arguments:   unsigned char *r:          pointer to the output serialized public key
*              const poly *pk:            pointer to the input public-key polynomial
*              const unsigned char *seed: pointer to the input public seed
**************************************************/
static void encode_pk(unsigned char *r, const poly *pk, const unsigned char *seed)
{
  int i;
  poly_tobytes(r, pk);
  for(i=0;i<NEWHOPE_SYMBYTES;i++)
    r[NEWHOPE_POLYBYTES+i] = seed[i];
}

/*************************************************
* Name:        decode_pk
* 
* Description: De-serialize the public key; inverse of encode_pk
*
* Arguments:   poly *pk:               pointer to output public-key polynomial
*              unsigned char *seed:    pointer to output public seed
*              const unsigned char *r: pointer to input byte array
**************************************************/
static void decode_pk(poly *pk, unsigned char *seed, const unsigned char *r)
{
  int i;
  poly_frombytes(pk, r);
  for(i=0;i<NEWHOPE_SYMBYTES;i++)
    seed[i] = r[NEWHOPE_POLYBYTES+i];
}

/*************************************************
* Name:        encode_c
* 
* Description: Serialize the ciphertext as concatenation of the
*              serialization of the polynomial b and serialization
*              of the compressed polynomial v
*
* Arguments:   - unsigned char *r: pointer to the output serialized ciphertext
*              - const poly *b:    pointer to the input polynomial b
*              - const poly *v:    pointer to the input polynomial v
**************************************************/
static void encode_c(unsigned char *r, const poly *b, const poly *v)
{
  poly_tobytes(r,b);
  poly_compress(r+NEWHOPE_POLYBYTES,v);
}

/*************************************************
* Name:        decode_c
* 
* Description: de-serialize the ciphertext; inverse of encode_c
*
* Arguments:   - poly *b:                pointer to output polynomial b
*              - poly *v:                pointer to output polynomial v
*              - const unsigned char *r: pointer to input byte array
**************************************************/
static void decode_c(poly *b, poly *v, const unsigned char *r)
{
  poly_frombytes(b, r);
  poly_decompress(v, r+NEWHOPE_POLYBYTES);
}

/*************************************************
* Name:        gen_a
* 
* Description: Deterministically generate public polynomial a from seed
*
* Arguments:   - poly *a:                   pointer to output polynomial a
*              - const unsigned char *seed: pointer to input seed
**************************************************/
static void gen_a(poly *a, const unsigned char *seed)
{
  poly_uniform(a,seed);
}


/*************************************************
* Name:        cpapke_keypair
* 
* Description: Generates public and private key 
*              for the CPA public-key encryption scheme underlying
*              the NewHope KEMs
*
* Arguments:   - unsigned char *pk: pointer to output public key
*              - unsigned char *sk: pointer to output private key
**************************************************/
void cpapke_keypair(unsigned char *pk,
                    unsigned char *sk)
{
  poly ahat, ehat, ahat_shat, bhat, shat;
  unsigned char z[2*NEWHOPE_SYMBYTES];
  unsigned char *publicseed = z;
  unsigned char *noiseseed = z+NEWHOPE_SYMBYTES;

  z[0] = 0x01;
  randombytes(z+1, NEWHOPE_SYMBYTES);
  shake256(z, 2*NEWHOPE_SYMBYTES, z, NEWHOPE_SYMBYTES + 1);

  gen_a(&ahat, publicseed);

  // 1. 비밀 다항식 s 생성 (중심이항분포)
  poly_sample(&shat, noiseseed, 0);
  
  // ================= [여기부터 추가] =================
  printf("\n[Alice - KeyGen]\n");
  printf(" 비밀 다항식 s의 계수: ");
  // 중심이항분포로 뽑혀 아주 작은 값(예: 0, 1, -1, 2 등)이 나올 것입니다.
  for(int i=0; i<5; i++) printf("%d ", shat.coeffs[i]);
  printf("\n");
  // ===================================================
  
  poly_ntt(&shat);

  // 2. 오류 다항식 e 생성 (중심이항분포)
  poly_sample(&ehat, noiseseed, 1);
  
  // ================= [여기부터 추가] =================
  printf(" 오류 다항식 e의 계수: ");
  for(int i=0; i<5; i++) printf("%d ", ehat.coeffs[i]);
  printf("\n");
  // ===================================================
  
  poly_ntt(&ehat);

  // 3. b = a * s + e 계산
  poly_mul_pointwise(&ahat_shat, &shat, &ahat);
  poly_add(&bhat, &ehat, &ahat_shat);

  // ================= [여기부터 추가] =================
  printf("공개 다항식 b (b = as + e)의 계수: ");
  // b는 q(12289) 범위 내의 큰 값들로 채워져 있을 것입니다.
  for(int i=0; i<5; i++) printf("%d ", bhat.coeffs[i]);
  printf("\n=============================\n");
  // ===================================================

  poly_tobytes(sk, &shat);
  encode_pk(pk, &bhat, publicseed);
}

/*************************************************
* Name:        cpapke_enc
* 
* Description: Encryption function of
*              the CPA public-key encryption scheme underlying
*              the NewHope KEMs
*
* Arguments:   - unsigned char *c:          pointer to output ciphertext
*              - const unsigned char *m:    pointer to input message (of length NEWHOPE_SYMBYTES bytes)
*              - const unsigned char *pk:   pointer to input public key
*              - const unsigned char *coin: pointer to input random coins used as seed
*                                           to deterministically generate all randomness
**************************************************/
void cpapke_enc(unsigned char *c,
                const unsigned char *m,
                const unsigned char *pk,
                const unsigned char *coin)
{
  poly sprime, eprime, vprime, ahat, bhat, eprimeprime, uhat, v;
  unsigned char publicseed[NEWHOPE_SYMBYTES];
  // ================= [여기부터 추가] =================
  printf("\n[Bob - 메시지 인코딩]\n");
  printf("원본 메시지 (m, 수식의 mu에 대응하는 입력 메시지) 32바이트: ");
  for(int i=0; i<NEWHOPE_SYMBYTES; i++) printf("%02X", m[i]);
  printf("\n");
  // ===================================================
  // 메시지 m을 다항식 v(이론의 mu)로 인코딩: 0은 0으로, 1은 q/2 근처로 변환됨
  poly_frommsg(&v, m);

  decode_pk(&bhat, publicseed, pk);
  gen_a(&ahat, publicseed);

  // Bob의 작은 임시 비밀/오류 다항식 생성 (중심이항분포)
  poly_sample(&sprime, coin, 0);
  poly_sample(&eprime, coin, 1);
  poly_sample(&eprimeprime, coin, 2);

  // ================= [여기부터 추가] =================
  printf("\n[Bob]\n");
  printf(" 비밀 다항식 s'의 계수: ");
  for(int i=0; i<5; i++) printf("%d ", sprime.coeffs[i]);
  printf("\n");

  printf(" 오류 다항식 e'의 계수: ");
  for(int i=0; i<5; i++) printf("%d ", eprime.coeffs[i]);
  printf("\n");

  printf(" 추가 오류 다항식 e''의 계수: ");
  for(int i=0; i<5; i++) printf("%d ", eprimeprime.coeffs[i]);
  printf("\n");
  
  printf("인코딩된 메시지 (mu)의 계수: ");
  // 이론상 0 또는 q/2 (약 6144) 근처의 값이 나와야 합니다.
  for(int i=0; i<5; i++) printf("%d ", v.coeffs[i]);
  printf("\n");
  // ===================================================

  poly_ntt(&sprime);
  poly_ntt(&eprime);

  // u = as' + e' 계산 (NTT 도메인에서 수행)
  poly_mul_pointwise(&uhat, &ahat, &sprime);
  poly_add(&uhat, &uhat, &eprime);

  // bs' 계산 및 역변환 (Inverse NTT)
  poly_mul_pointwise(&vprime, &bhat, &sprime);
  poly_invntt(&vprime);

  // v = bs' + e'' + mu (메시지 추가) 계산 (일반 도메인에서 수행)
  poly_add(&vprime, &vprime, &eprimeprime);
  poly_add(&vprime, &vprime, &v); // add message

  // ================= [여기부터 추가] =================
  printf("최종 암호문 v (bs' + e'' + mu)의 계수: ");
  for(int i=0; i<5; i++) printf("%d ", vprime.coeffs[i]);
  printf("\n=============================\n");
  // ===================================================

  encode_c(c, &uhat, &vprime);
}


/*************************************************
* Name:        cpapke_dec
* 
* Description: Decryption function of
*              the CPA public-key encryption scheme underlying
*              the NewHope KEMs
*
* Arguments:   - unsigned char *m:        pointer to output decrypted message
*              - const unsigned char *c:  pointer to input ciphertext
*              - const unsigned char *sk: pointer to input secret key
**************************************************/
void cpapke_dec(unsigned char *m,
                const unsigned char *c,
                const unsigned char *sk)
{
  poly vprime, uhat, tmp, shat;

  // 1. 비밀키 s를 불러옴
  poly_frombytes(&shat, sk);

  // 2. 암호문 c에서 u와 v를 분리
  decode_c(&uhat, &vprime, c);
  
  // 3. u * s 계산 (NTT 도메인에서 곱셈 후 Inverse NTT로 복원)
  poly_mul_pointwise(&tmp, &shat, &uhat);
  poly_invntt(&tmp); // 여기서 tmp가 이론상의 v' (us)가 됩니다.

  // ================= [여기부터 추가] =================
  printf("\n[Alice]\n");
  printf("Alice가 계산한 v' (us)의 계수: ");
  // 앞서 Bob 단계에서 출력한 v의 계수와 값이 얼마나 비슷한지 비교해 보세요!
  for(int i=0; i<5; i++) printf("%d ", tmp.coeffs[i]);
  printf("\n");
  // ===================================================

  // 4. (v - us) 계산: 메시지와 작은 오류만 남기는 과정
  poly_sub(&tmp, &vprime, &tmp);

  // ================= [여기부터 추가] =================
  printf("오류가 포함된 복원 메시지 (v - v')의 계수: ");
  // 이론상 0 근처이거나 q/2(약 6144) 근처의 값이 나와야 합니다.
  for(int i=0; i<5; i++) printf("%d ", tmp.coeffs[i]);
  printf("\n=============================\n");
  // ===================================================

  // 5. 0 또는 q/2 근처인지 판단하여 최종 비트(0 또는 1)로 디코딩
  poly_tomsg(m, &tmp);
  // ================= [여기부터 추가] =================
  printf("디코딩된 최종 메시지 (m, 수식의 mu에 대응하는 입력 메시지): ");
  for(int i=0; i<NEWHOPE_SYMBYTES; i++) printf("%02X", m[i]);
  printf("\n=============================\n");
  // ===================================================
}