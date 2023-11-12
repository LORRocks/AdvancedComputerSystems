#include <immintrin.h>
#include <string>
#include <iostream>
#include <cstring>


int using_simd = 0;
//this is our two functions that make use of simd to speed up the dict encoder

//a string comparision

int simd_strcmp_helper(const char * one, const char * two){
	__m128i s1 = _mm_loadu_si128((__m128i_u*)(one));	
	__m128i s2 = _mm_loadu_si128((__m128i_u*)(two));	
	return _mm_cmpistro(s1, s2, _SIDD_CMP_EQUAL_EACH | _SIDD_UBYTE_OPS);
}

int simd_strcmp(std::string & one, std::string & two){
	if(using_simd){
		if(one.length() > 16 || two.length() > 16){
			std::cout << "OVER !&";
		}
		return simd_strcmp_helper(one.c_str(), two.c_str());
	}else{
		return one.compare(two);
	}
}

//prefix string comparision
int simd_prefix(const char * a, const char *b){
	    size_t len_b = strlen(b);

	    __m128i avec, bvec;
	    __m128i zero_mask = _mm_setzero_si128();

	    for (size_t i = 0; i < len_b; i += 16) {
		avec = _mm_loadu_si128((__m128i *)(a + i));
		bvec = _mm_loadu_si128((__m128i *)(b + i));

		__m128i len_mask = _mm_cmpgt_epi8(_mm_set1_epi8(len_b - i), _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
		avec = _mm_and_si128(avec, len_mask);
		bvec = _mm_and_si128(bvec, len_mask);

		__m128i greater_than_mask = _mm_cmpgt_epi8(avec, bvec);
		__m128i less_than_mask = _mm_cmplt_epi8(avec, bvec);

		if (_mm_test_all_zeros(greater_than_mask, greater_than_mask)
		    && _mm_test_all_zeros(less_than_mask, less_than_mask)) { // Equal
		    continue; // Equal in this iteration, continue to next
		} else {
		    return 1;
		}
	    }
	return 0;
}

//Compares the string in one up to the length of two
//two is the prefix, one is the string
int strcmp_prefix(std::string & one, std::string & two){
	if(using_simd){
		return simd_prefix(one.c_str(), two.c_str());
	}else{
		if(one.length() != two.length()){return 1;}
		for(int i = 0; i < two.length();i++){
			if(one[i] != two[i]){
				return 1;
			}
		}
		return 0;
	}
}

//our custom hash function using SIMD
//it's a rolling polynominal string hash
std::hash<std::string> hasher;
		char weights[] = {1,2,4,8,16,32,64,127,1,2,4,8,16,32,64,127,1,2,4,8,16,32,64,127,1,2,4,8,16,32,64,127};
struct KeyHasher
{
	std::size_t operator()(const std::string & s) const
	{
		if(1){
			return hasher(s);
		}

		__m256i w = _mm256_loadu_si256((__m256i_u*)weights);
		
		//pad the string so we don't have to worry about overreading data
		const char* c_str  = s.c_str();
		char s1[32];
		int i = 0;
		while(i < 32 & c_str[i] != 0){
			s1[i] = c_str[i];	
			i++;
		}
		while(i < 32){
			s1[i] = 0;
			i++;
		}

		__m256i sr = _mm256_loadu_si256((__m256i_u*)c_str);
		__m256i result = _mm256_maddubs_epi16(w,sr);

		//extract and combine the results
		int results[8];
		_mm256_storeu_si256((__m256i*) &results[0], result);	

		int acc = 0;

		for(int i = 0; i < s.length();i += 4){
			acc += results[i];
		}
		

		return acc;
	};
};


