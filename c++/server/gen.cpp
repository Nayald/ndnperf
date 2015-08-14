//Generate an RSA key pair, sign a message and verify it using crypto++ 5.6.1 or later.
//By Tim Sheerman-Chase, 2013
//This code is in the public domain and CC0
//To compile: g++ gen.cpp -lcrypto++ -o gen

#include <string>
using namespace std;
#include <crypto++/rsa.h>
#include <crypto++/osrng.h>
#include <crypto++/base64.h>
#include <crypto++/files.h>
using namespace CryptoPP;

void GenKeyPair(string s, int size)
{
	// InvertibleRSAFunction is used directly only because the private key
	// won't actually be used to perform any cryptographic operation;
	// otherwise, an appropriate typedef'ed type from rsa.h would have been used.
	AutoSeededRandomPool rng;
	InvertibleRSAFunction privkey;
	privkey.Initialize(rng, size);

	// With the current version of Crypto++, MessageEnd() needs to be called
	// explicitly because Base64Encoder doesn't flush its buffer on destruction.
	Base64Encoder privkeysink(new FileSink((s+".pri").c_str()));
	privkey.DEREncode(privkeysink);
	privkeysink.MessageEnd();
	 
	// Suppose we want to store the public key separately,
	// possibly because we will be sending the public key to a third party.
	RSAFunction pubkey(privkey);
	
	Base64Encoder pubkeysink(new FileSink((s+".pub").c_str()));
	pubkey.DEREncode(pubkeysink);
	pubkeysink.MessageEnd();
	cout << size << endl;
}

int main(int argc, char* argv[])
{
	if(argc>2)GenKeyPair(argv[1],atoi(argv[2]));
	return 0;
}

