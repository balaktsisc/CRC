#include <iostream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <fstream>

using namespace std;

/**
 * @param num An array of binary digits.
 * @param len The len of the array.
 * @return Decimal representation of the number represented by the array elements.
 */
int dec(const int *num, int len) {
    int total = 0;
    for(int i = 0; i < len; i++)
        total += num[i] * (int) pow(2,(len - i - 1));
    return total;
}

/**
 * @param num An array of binary digits.
 * @param start The position of the considered first element (digit) of the array (number).
 * @param end The position of the considered last element (digit) of the array (number).
 * @return Binary representation of the number represented by the array elements.
 */
string bin(const int *num, int start, int end) {
    string total;
    for(int i = start; i < end; i++) {
        total.append(to_string(num[i]));
    }
    return total;
}

/**
 * This function calculates the FCS of the number that param packet array represents;
 * it uses the dec function to get the decimal format of packet and key arrays and divides
 * them as following: operates bitwise XOR for the same-length (=key_len) bits of the two
 * numbers and keeps the (key_len-1)-bit remainder for the next XOR operation, in which
 * one more bit is pulled down from the packet number (dividend) in the end of the remainder,
 * so as the remainder is long-enough (key_len). When packet's bits are all pulled down, the
 * remainder is the FCS.
 * Note: floor(log2l(x)) + 1 = bit length of int x.
 * @param key The pivot number for checks represented by array's elements (binary digits).
 * @param key_len The key-array length.
 * @param packet The transferred packet to be sent represented by array's elements (binary digits)
 * @param packet_len The packet-array length.
 * @return Frame Check Sequence of the transferred packet.
 */
int fcs_calc(const int *key, int key_len, const int *packet, int packet_len) {
    int r = dec(packet, packet_len), p = dec(key, key_len);
    while(r && p && floor(log2l(r)) >= floor(log2l(p)))
        r = r ^ p << (int)(floor(log2l(r)) - floor(log2l(p)));
    return r % (int) pow(2,key_len-1);
}

/**
 * This function produces randomly sender packets, appends in the ending bits the FCS for each one
 * and then creates a receiver message based (indeed copying) on the sender packet, but with a probability
 * of reversing each of its bits. Then it checks if the receiver message produces 0 or 2^(key_len-1) as FCS,
 * so it seems to be transmitted right, or else, an error has been detected.
 * @param data_len The number of digits of the sending packet (bare data frame).
 * @param key The pivot number for checks represented by array's elements (binary digits).
 * @param key_len The key-array length.
 * @param ber The Bit Error Rate (BER); if a random number is less than BER, then the copying bit gets reversed.
 * @param counted_errors The counter of errors occurred during packet transmissions; adds 1 if at least an error bit occurred on a receiving packet.
 * @param output The text file to store the transferred message info.
 * @return If an error was produced (1) or not (0), during a single packet transmission.
 */
int crc(int data_len, const int *key, int key_len, float ber, unsigned long long &counted_errors, ofstream &output) {
    // Total message of data_len+key_len bits; each sender-array element represents a binary digit.
    int packet_len = data_len + key_len - 1;
    int sender[packet_len], flag = 0;

    // Message to be sent of data_len bits; each binary digit is set randomly.
    for(int i = 0; i < data_len; i++)
        sender[i] = rand() % 2;

    // Shifting key_len-bits; setting the last key_len-bits (a.data_len.a. elements) to 0.
    for(int i = data_len; i < packet_len; i++)
        sender[i] = 0;

    int fcs = fcs_calc(key, key_len,sender, packet_len);
    for(int i = packet_len - 1; i >= data_len && fcs >= 0; i--) {
        sender[i] = fcs % 2;
        fcs /= 2;
    }
    /* Creating the receiver message.
     * Copies the sender message with probabilistic certainty (uniform distribution); if a random float is less than BER,
     * then we consider the opposite binary digit to be transferred.
     */
    int receiver[packet_len];
    for(int i = 0; i < packet_len; i++) {
        if (static_cast <float> (rand()) / static_cast <float> (RAND_MAX) < ber) {
            receiver[i] = sender[i] ? 0 : 1;
            flag++; // Notify if at least one bit has been changed => error occurred.
        }
        else
            receiver[i] = sender[i];
    }

    if(flag) counted_errors++;
    bool error = fcs_calc(key, key_len,receiver, packet_len) != 0;

//    output << "MESSAGE: " << bin(sender,0,data_len) << "\tFCS: " << bin(sender,data_len,packet_len) << "\tERROR: " << (error ? "TRUE" : "FALSE") << endl;
    return error ? 1 : 0;
}

/**
 * Main function; runs a certain number of test-cases for crc function and counts the amounts of occurred and detected errors.
 */
int main() {
    srand(time(nullptr));

    ofstream messages;
    messages.open("Messages.txt");
    
    int k = 20, p[6] = {1,1,0,1,0,1};
    
    unsigned long long test_cases = 10000000, detected_errors = 0, real_errors = 0;
    for(unsigned long long i = 0; i < test_cases; i++)
        detected_errors += crc(k,p,6,0.001,real_errors,messages);

    messages.close();

    cout << "--- Number of transmitted packets: " << test_cases << " | P = " << bin(p,0,6) << " | Packet Length = " << k + 6 - 1 << endl;
    cout << "Errors detected: " << detected_errors << endl;
    cout << "Errors produced: " << real_errors << endl;
    cout << "Rate of produced errors totally: (%) " << (float) (real_errors) / (float) (test_cases) * 100.0 << endl;
    cout << "Success rate of detection: (%) " << (float) (detected_errors) / (float) (real_errors) * 100.0 << endl;
    cout << "Miss rate of detection: (%) " << (float) (real_errors-detected_errors) / (float) (real_errors) * 100.0 << endl;
    cout << "Rate of detected errors totally: (%) " << (float) (detected_errors) / (float) (test_cases) * 100.0 << endl;
    cout << "Rate of non-detected errors totally: (%) " << (float) (real_errors-detected_errors) / (float) (test_cases) * 100.0 << endl;
    cout << "---";
    return 0;
}
