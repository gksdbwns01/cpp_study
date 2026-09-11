/* This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * See LICENSE for complete information.
 */

#define MAIN 1

#include <sys/time.h>

#include "rlwe_kex.h"

#define TRIALS 1000

int main(int argc, char *argv[]) 
{

	/*Exclusively For Alice*/
	RINGELT s_alice[2*m]; /* Alice's Private Key */
	uint64_t mu_alice[muwords]; /* Alice's recovered mu */

	/*Exclusively For Bob*/
	uint64_t mu_bob[muwords]; /* Bob's version of mu */

	/*Information that gets shared by Alice and Bob*/
	RINGELT b_alice[m]; /* Alice's Public Key */
	RINGELT u[m]; /* Bob's Ring Element from Encapsulation */
	uint64_t cr_v[recwords]; /* Cross Rounding of v */

	KEM1_Generate(s_alice, b_alice);

	KEM1_Encapsulate(u, cr_v, mu_bob, b_alice);

	KEM1_Decapsulate(mu_alice, u, s_alice + m, cr_v);

	int i, flag = 1;	
	for (i = 0; i < muwords; ++i) flag &= (mu_alice[i] == mu_bob[i]);
	if (flag) {
		printf("Successful Key Agreement!\n");
	}
	else {
		printf("Failure in Key Agreement :-(\n");
		for (i = 0; i < muwords; ++i) {
			printf("%"PRIu64"\t", mu_alice[i]);
		}
		printf("\n");
		for (i = 0; i < muwords; ++i) {
			printf("%"PRIu64"\t", mu_bob[i]);
		}
		printf("\n");

		exit(-1);
	}

	printf("Alice's version of mu\n");
	for (i = 0; i < muwords; ++i) {
		printf("%lx ", mu_alice[i]);
	}
	printf("\n\n");

	printf("Bob's version of mu\n");
	for (i = 0; i < muwords; ++i) {
		printf("%lx ", mu_bob[i]);
	}

	printf("\n\n");

	printf("Running %d trials\n", TRIALS);
	int ii;
	for (ii = 0; ii < TRIALS; ++ii) {
		memset(s_alice, 0, 2*m*sizeof(RINGELT));
		memset(b_alice, 0, m*sizeof(RINGELT));
		memset(u, 0, m*sizeof(RINGELT));
		memset(mu_alice, 0, muwords*sizeof(uint64_t));
		memset(mu_bob, 0, muwords*sizeof(uint64_t));
		memset(cr_v, 0, recwords*sizeof(uint64_t));

		// 1. 앨리스가 비밀키 s와 오류 e를 생성하여 공개키 b = a*s + e mod q 계산
        KEM1_Generate(s_alice, b_alice);
        printf("\n[Step 1. 공개키 생성 (b = as + e)]\n");
        printf(" -> 앨리스의 공개키 b의 첫 번째 계수: %lu\n", (unsigned long)b_alice[0]);

        // 2. 밥이 임시 랜덤 벡터 r을 선택하여 u = a*r + e1, v = b*r + μ + e2 계산
        // (Peikert 방식에서는 v 전체를 보내지 않고, 0과 q/2 중 어디에 가까운지 영역 판별용 힌트 cr_v만 보냄)
        KEM1_Encapsulate(u, cr_v, mu_bob, b_alice);
        printf("\n[Step 2. 암호화 및 힌트 전송 (u = ar + e1, 힌트 cr_v)]\n");
        printf(" -> 밥이 보낸 암호문 u의 첫 번째 계수: %lu\n", (unsigned long)u[0]);
        printf(" -> 밥이 보낸 메시지(μ) 영역 판별용 교차점 힌트(cr_v[0]): %016lx\n", cr_v[0]);
        
        // 3. 앨리스가 받은 u와 자신의 비밀키 s를 곱해 v - s*u = μ + error 계산 후 메시지 복구
        KEM1_Decapsulate(mu_alice, u, s_alice + m, cr_v);
        printf("\n[Step 3. 복호화 (v - su = μ + error)]\n");
        printf(" -> 앨리스가 0과 q/2 영역을 판별하여 메시지(μ) 복구 완료\n\n");


		flag = 1;	
		for (i = 0; i < muwords; ++i) flag &= (mu_alice[i] == mu_bob[i]);
		if (!flag) {
			printf("Failure in Key Agreement on trial %d\n", ii);
			exit(-1);
		}
	}
	printf("%d trials completed successfully\n", TRIALS);

}


