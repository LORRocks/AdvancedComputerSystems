#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <queue>
#include <atomic>
#include <vector>
#include <fstream>

#include <chrono>

#include "simd.cpp"
#include "dict.cpp"

#include "fastpfor/codecfactory.h"
#include "fastpfor/deltautil.h"

void encodeFile(std::string inputFileName, std::string outputFileName){
	//Start by generating our dict from the file
	std::ifstream inputFile;
	inputFile.open(inputFileName, std::ifstream::in);
	std::unordered_map<std::string,int,KeyHasher> * dict = generateDict(inputFile);
	inputFile.close();

	//Now we are going through our file again and generating our vector of integers
	inputFile.open(inputFileName, std::ifstream::in);
	std::vector<unsigned int> uncompressed;
	uncompressed.reserve(dict->size()*2);	
	std::string line;
	while(1){
		if(getline(inputFile, line)){
			unsigned int encoded = (*dict)[line];
			uncompressed.push_back(encoded);
		}else{break;}
	}

	size_t uncompressedSize = uncompressed.size();

	//Compress our vector of integers in the compressed vector, of length compressedSize
	size_t N = uncompressed.size();
	using namespace FastPForLib;
	CODECFactory factory;	
 	IntegerCODEC &codec = *factory.getFromName("simdfastpfor256");
  	std::vector<uint32_t> compressed_output(N + 1024);

	size_t compressedsize = compressed_output.size();
	codec.encodeArray(uncompressed.data(), uncompressed.size(), compressed_output.data(),
                    compressedsize);

	  compressed_output.resize(compressedsize);
	  compressed_output.shrink_to_fit();


	//Now we we will write out to our compressed file in the following format
	/**
	 * lengthOfDict : int
	 * data:
	 * 	null terminated string : int
	 * lengthOfDataRaw : int
	 * lengthOfDataCompressed : int
	 * 	compressedIntegerStream
	 */
	std::ofstream outputStream;
	outputStream.open(outputFileName, std::ios::out | std::ios::binary);

		//output our dict
	int size = dict->size();
	outputStream.write((char *)&size, sizeof(size));
	for(auto i : *dict){
		std::string str = i.first;
		const char * raw = str.c_str();
		int value = i.second;
		outputStream.write(raw, strlen(raw) + 1);
		outputStream.write((char *)&value,sizeof(value));
	}


		//output our compressed value
	outputStream.write((char *)&uncompressedSize, sizeof(uncompressedSize));
	compressedsize = compressed_output.size();
	outputStream.write((char *)&compressedsize, sizeof(compressedsize));
//	const char * marker = "mark";
//	outputStream.write(marker,5);
	for(size_t i = 0; i < compressed_output.size();i++){
		uint32_t value = compressed_output[i];
		outputStream.write((char*)&value,sizeof(value));
	}


	outputStream.close();

	//We have now succesfully encoded the file!

}

//returning a dict of strings to integers, and a map of integers -> line numbers
void read_compressed_file(std::string inputFileName, std::unordered_map<std::string, int,KeyHasher> & dict, std::unordered_map<int, std::vector<int>> & indexes){

	//Open the file
	std::ifstream input;
	input.open(inputFileName, std::ios::in | std::ios::binary);


	//Read in the dict size
	int dictSize = 0;
	input.read((char*)&dictSize, sizeof(dictSize));
	dict.reserve(dictSize);
	//Read in the dict
	for(int i = 0; i < dictSize;i++){
		std::string s;
		std::getline(input, s, '\0');
		int value = 0;
		input.read((char*)&value, sizeof(value));
		dict[s] = value;	
	}

	//Read in the data
	size_t rawDataSize = 0;
	size_t compressedDataSize = 0;
	input.read((char*)&rawDataSize, sizeof(rawDataSize));
	input.read((char*)&compressedDataSize, sizeof(compressedDataSize));
	//char buffer[10];
	//input.read(buffer, 5);
	std::vector<unsigned int> compressedData;
	compressedData.reserve(compressedDataSize + 1024);
	for(int i = 0; i < compressedDataSize;i++){
		unsigned int value = 9;
		input.read((char*)&value, sizeof(value));
		compressedData.push_back(value);
	}
	//Uncompress the data
	using namespace FastPForLib;
	CODECFactory factory;	
 	IntegerCODEC &codec = *factory.getFromName("simdfastpfor256");

	  std::vector<uint32_t> uncompressed(rawDataSize);
	  size_t recoveredsize = uncompressed.size();
	  //
	  codec.decodeArray(compressedData.data(), compressedData.size(),
			    uncompressed.data(), recoveredsize);
  	uncompressed.resize(recoveredsize);

	//Now that we have the uncompressed data, we could simply leave it as is, but encoding it into a better data structure gives a much better search value
	for(int i = 0 ; i < recoveredsize;i++){
		int line = uncompressed[i];
		if(indexes.count(line) == 0){
			indexes[line] = std::vector<int>();
			indexes[line].push_back(i);
		}else{
			indexes[line].push_back(i);
		}
	}


	input.close();

}

void search(std::string searchTerm,std::unordered_map<std::string, int, KeyHasher> & dict, std::unordered_map<int,std::vector<int>> & indexes,  std::vector<int> & results){
	//This is a very simple search
	//If our data structures were constructed properly, then our search will be in a O(1) lookup time
	//Start by determining if we even have a search term
	int value = dict[searchTerm];
	results = indexes[value];
	return;
}

void searchPrefix(std::string searchPrefix,std::unordered_map<std::string, int, KeyHasher> & dict, std::unordered_map<int,std::vector<int>> & indexes,  std::vector<int> & results){
	//Doing this efficently involves running thorough our dict, and finding which values match the prefixs, and then running it through our search
	std::vector<int> searchTerms;	
	for(auto i : dict){
		std::string str = i.first;
		int value = i.second;
		if(strcmp_prefix(str, searchPrefix) == 0){
			searchTerms.push_back(i.second);
		}
	}
	for(int i = 0; i < searchTerms.size();i++){
		for(int j = 0; j < indexes[searchTerms[i]].size();j++){
			results.push_back(indexes[searchTerms[i]][j]);
		}		
	}
}

void searchVanilla(std::string fileName, std::string value, std::vector<int> & results){
	//Here we have a naive implementation of a search algorithm, run through the entire file and compare each value
	std::ifstream input;
	input.open(fileName, std::ios::in);

	std::string line;
	int lineNumber = 0;
	while(input){
		getline(input, line);
		if(line.compare(value) == 0){
			results.push_back(lineNumber);
		}
		lineNumber++;
	}

	input.close();
}		

void searchPrefixVanilla(std::string fileName, std::string prefix, std::vector<int> & results){
	//Here we have again a simple implementation running through the file, comparing each prefix
	//Here we have a naive implementation of a search algorithm, run through the entire file and compare each value
	std::ifstream input;
	input.open(fileName, std::ios::in);

	std::string line;
	int lineNumber = 0;
	while(input){
		getline(input, line);
		if(strcmp_prefix(line, prefix) == 0){
			results.push_back(lineNumber);
		}
		lineNumber++;
	}

	input.close();
}

int main(int argc, char ** argv){
	/**
	 * usage example
	 * ./a.out flags mode
	 * flags : 
	 * -s use simd instructions
	 * -t set thread amount from (-t for example)
	 * mode :
	 * compress inputFile outputFile
	 * readraw input
	 * readbin input
	 */

	if(argc < 2){
		std::cout << "Incorrect usage" << std::endl;
		return 1;
	}	
	//start reading in the flags
	int argNum = 1;
	while(true){
		if(strcmp("-s", argv[argNum]) == 0){
			std::cout << "SIMD enabled" << std::endl;
			using_simd = 1;
			argNum++;
		}else if(strcmp("-t", argv[argNum]) == 0){
			argNum++;
			MAXTHREAD = atoi(argv[argNum]);
			argNum++;
			std::cout << "Mulithreading set to " << MAXTHREAD << std::endl;
		}else{
			break;		
		}	
	}
	//read in the mode
	char * mode = argv[argNum];
	std::cout << "Mode : " << mode << std::endl;
	int compressed_file = 0;
	std::unordered_map<std::string, int, KeyHasher> dict;
	std::unordered_map<int, std::vector<int>> indexes;
	char * inputFile;
	if(strcmp(mode, "compress") == 0){
		if(argNum + 3 > argc){
			std::cout << "Invalid usage of compress mode" << std::endl;
			return 1;
		}
		inputFile = argv[argNum + 1];
		char * outputFile = argv[argNum + 2];
	
		std::cout << "Compressing file " << inputFile << " into " << outputFile << std::endl;	

		auto start = std::chrono::high_resolution_clock::now();
		encodeFile(inputFile, outputFile);		
		auto stop = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
		if(duration.count() > 0){
			std::cout << "File compression complete in " << duration.count() << "s" << std::endl;
		}else{
			auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
			std::cout << "File compression complete in " << duration2.count() << "ms" << std::endl;
		}


		return 0;
	}else if (strcmp(mode, "readraw") == 0){
		compressed_file = 0;
		inputFile = argv[argNum + 1];
	}else if (strcmp(mode, "readbin") == 0){
		compressed_file = 1;
		
		auto start = std::chrono::high_resolution_clock::now();
		read_compressed_file(argv[argNum + 1], dict, indexes);
		auto stop = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
		if(duration.count() < 3){
			std::cout << "Compressed file read in " << duration.count() << "s" << std::endl;
		}else{
			auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
		}

	}else{
		std::cout << "Invalid mode" << std::endl;
		return 0;
	}

	//If we're here, we're in interactive mode
	std::cout << "Use search <term> for full search" << std::endl;
	std::cout << "Use prefix <term> for prefix search" << std::endl;
	std::cout << "Use exit to exit" << std::endl;

	while(1){
		std::string input;
		std::cout << ">>> ";
		std::cin >> input;
		if(strcmp(input.c_str(), "exit") == 0){
			return 0;
		}
		if(strcmp(input.c_str(), "search") == 0){
			std::vector<int> results;
			std::string search_term;
			std::cin >> search_term;
			std::cout << "Searching for " << search_term << std::endl;
			auto start = std::chrono::high_resolution_clock::now();
			if(compressed_file){
				search(search_term, dict, indexes, results);
			}else{
				searchVanilla(inputFile, search_term, results);
			}
			auto stop = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
			if(duration.count() > 10){
				std::cout << "Search time: " << duration.count() << "s" << std::endl;
			}else{
				auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

				if(duration2.count() > 10){
					std::cout << "Search time: " << duration2.count() << "ms" << std::endl;
				}else{
					auto duration3 = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

					std::cout << "Search time: " << duration3.count() << "ns" << std::endl;
				}
			}
			
			if(results.size() == 0){
				std::cout << "No results found" << std::endl;
			}else{
				if(results.size() < 15){	
				std::cout << "Lines found : ";
				for(int i = 0; i < results.size();i++){
					std::cout << results[i] << " ";
				}
				std::cout << std::endl;
				}else{
					std::cout << results.size() << " lines found " << std::endl;
				}
			}

			continue;
		}
		if(strcmp(input.c_str(), "prefix") == 0){
			std::vector<int> results;
			std::string search_term;
			std::cin >> search_term;
			std::cout << "Searching for prefix " << search_term << std::endl;
			auto start = std::chrono::high_resolution_clock::now();
			if(compressed_file){
				searchPrefix(search_term, dict, indexes, results);
			}else{
				searchPrefixVanilla(inputFile, search_term, results);
			}
			auto stop = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
			if(duration.count() > 10){
				std::cout << "Search time: " << duration.count() << "s" << std::endl;
			}else{
				auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
				std::cout << "Search time: " << duration2.count() << "ms" << std::endl;
			}
			
			if(results.size() == 0){
				std::cout << "No results found" << std::endl;
			}else{
				if(results.size() < 15){	
				std::cout << "Lines found : ";
				for(int i = 0; i < results.size();i++){
					std::cout << results[i] << " ";
				}
				std::cout << std::endl;
				}else{
					std::cout << results.size() << " lines found " << std::endl;
				}
			}
			continue;
		}
		std::cout << "Invalid command" << std::endl;
	}
}
