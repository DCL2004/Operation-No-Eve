# Operation No Eve
By Daniel Lin and Irene Xu

## Introduction
- Eavesdropping has long been one of the primary security problems in data transmission, especially for those lower-level communications that lack built-in protection. The I2C circuit can be used for short-distanced communication between devices, but its simplicity also makes it vulnerable to eavesdropping.
- This system builds a more secure communication between two devices. It provides a layer of protection on the message being transmitted by adding encryption and decryption processes before and after it gets to the I2C circuit. This process makes the exposed message safer to eavesdroppers because without the encryption key, the eavesdroppers would need to guess among the countless possibilities. The goal is to stop the eavesdropper, who receives the message along with the intended receiver, from understanding it.
- What we've accomplished are setting up successful communication between two boards, implementing a new device to act as the eavesdropper, transmitting plaintext messages between two boards to show successful connection, and adding encryption and decryption methods to protect data.
- Along this way, we learned the insecurities of device communication through shared circuits and the challenges of implementing security measures on boards without built-in protection.

## System Overview
- Phrase 1, communication
- Alice send message to Bob, which goes through the I2C channel.
  - ![image](https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/IMG_8597.png)
  - ![IMG_2038](https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/IMG_2038.jpg)


- Phrase 2, eavesdropping
- Eve starts eavesdropping for information on the I2C board since it's a shared circuit. Because the message is not encrypted, Eve can recover the original message.
  - ![image](https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/IMG_8598.png)
  - ![IMG_2041](https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/IMG_2041.jpg)


- Phrase 3, defense
- Alice and Bob build defense system against Eve by adding encoding and decoding processes prior to sending and receiving messages. Now Eve can still receive the encoded text, but without the given encryption key, Eve cannot recover the original message.
  - ![image](https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/IMG_8599.png)

- Video: Demonstrate Phase 1 (the unsecure method) and Phase 3 (the completed, secure method).
  - unsecure I2C
  - This video shows message transmission between Alice and Bob without encryption. The several short messages we want to send are "ECE3140", "HELLO", “PHASE 1", and "I2C WORKS". On the left side, it's supposed to show the messages Bob received, so that we can verify that the system between Alice and Bob is working correctly. On the right side, it shows the messages Eve copies down from the shared communication of the I2C circuit. By observing each character on the right side, we can see that Eve can successfully intercept the messages and understand them, just like Bob.

<video src="https://github.com/DCL2004/Operation-No-Eve/raw/main/demo/video_unsecure_I2C.mp4" controls></video>

  - secure I2C
  - This video shows the same message transmission process, but with encryption and decryption added. With the given encryption key, "ECE3140" is encoded to "BCB3140", "HELLO" is "LBOOY", and "PHASE 1" is "HLZRB 1". On the left side, it still shows the encoded message that Bob receives and the original message that Bob recovers using the decryption key. This shows that the encryption method is working correctly and the data transmission between Alice and Bob is also working. On the right side, it shows the messages Eve intercepts. However, Eve can only intercepts the encoded messages such as "LBOOY" and "HLZRB 1". Since Eve was not given the keys, Eve wouldn't understand the meaning of by intercepting the messages from the shared communication I2C circuit, which achieves our goal.
    
<video src="https://github.com/DCL2004/Operation-No-Eve/raw/main/demo/video_secure_I2C.mp4" controls></video>


## System Description
- Board A: master (Alice), sends messages
  - Alice acts as the I2C master of the system. It generates, encrypts, and sends { 0xAB, counter } to slave 0x48 every ~100 ms. It uses D14 for SDA and D15 for SCL, and its GPIO pins are initialized as below:
  - <img width="388" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/a6733788-7a93-4466-8419-9849f3bf27d3.png" />
  - In send_text(const char *msg, uint8_t len), it waits for the bus to become available and then set the MST and TX bits in I2C using:
  - <img width="292" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/27b79ae4-f4ff-4aca-83e7-fa5a66ba0ed3.png" />
  - Then it sends the message one bit at a time to Bob's address. The MSG_MARKER allows Bob to distinguish the sent bit from the rest. During this process, Alice checks for TCF (transfer complete flag) and RXAK after sending each bit, if either is triggered, the process would terminate immediatley. As shown in the screenshot, in the while loop, it decrements t and stays inside when transfer is not complete and t is not zero. The following is the code for i2c_wait_tcf_ack.
  - <img width="224" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/e54c5a84-aa51-45d6-8fbf-b41d39adbbc6.png" />

- Board B: slave (Bob), receives messages
  - Bob acts as the slave of the system. It receives and decrypts the bit messages got from Alice, and display the results through UART serial output. It uses D14 for SDA and D15 for SCL as well. The I2C function is initialized as below:
  - <img width="358" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/90ce7784-5206-465a-942a-cf1f47ff31db.png" />
  - Since it needs to display the results, Bob also needs to initialize UART ports. The UART ports need to configure transmite and receive pins and enable serail communication. For UART transmission, we use uart_putc, which waits until the transmission data register is empty and then opens up for receiving the next character.
  - To analyze the received data, it first waits until the master addresses the slave by its address 0x48u. Then it let the slave enter receive mode and clear the internal state before receiving data. As long as the I2C tranmission is active, Bob would continue receiving data in a while loop. Inside the while loop, Bob waits until there's a next character, the I2C transmission ends, or reaches timeout limit. After the while loop, Bob reads the received character and stores it inside an array. The full process when I2C is active is shown as below:
  - <img width="314" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/f620a6ed-e886-416e-b623-05b19bc65482.png" />
  - After decrypting the data, Bob uses display_packet(const uint8_t *buf, uint8_t n) to display the message marker and bit pair for each character. It would display both the received encoded message and the decrypted original one in the form of strings.

- Board C: eavesdropper (Eve), captures messages without participating in the data transmission process
  - Eve is the eavesdropper in this system. Unlike Alice and Bob, Eve doesn't participate in the I2C communication. It configures PTE0 and PTE1 as GPIO inputs, tracking the I2C traffics through SDA and SCL and printing them over UART0. Eve and Bob both receive the message, so most parts of their functions are the same. However, the main difference is that since Eve doesn't actively participate in the process, it waits for the SCL rising edge which indicates a new bit is received. When SCL is on rising edge, SDA changes from low to high for start and from high to low for stop.
  - Using the start and stop condition, Eve converts the bits into readable characters. For all received message stored in frame_buf, it prints the data and the bytes in hexidecimal. If the message is the first bit, Eve would use uart_puts("ADDR=0x"); uart_hex8(val >> 1u); uart_putc((val & 1u) ? 'R' : 'W'); to print the slave address and Read/Write bit. The full code snippet is shown as below:
  - <img width="224" alt="image" src="https://github.com/DCL2004/Operation-No-Eve/blob/main/demo/38a805f9-7ee2-434d-9091-6291f468d4e3.png" />

- I2C circuit:
  - The three FRDM-KL46Z boards are connected on a breadboard using shared common I2C circuit. The 3 board arrangements are shown in the picture of Phase 2 in System Overview section. All boards share the common SDA for data and SCL for clock, and can observe the activities on these two pins, which allows Eve to observe the message without participating in the data transmission process. We used 2 external resistors of 6.8kohm, connected to SDA and SCL, to pull the circuit up to satisfy the I2C circuit's active low logic. These resistors allow the 3 devices to share the same circuit safely, preventing constantly changing data and making stable communications across all devices.

- Encryption and decryption:
  - We implement a substitution cipher for all 26 alphabets, including uppercase and lowercase. The encryption and decryption keys are generated beforehand and put inside the coding snippets. On Alice's side, message is transformed into cipher, which would be passed through the I2C circuit, which Eve would capture. When Bob receives the cipher, it would use the given decryption key to recover the original message.
  - Our given key is static const char key[] = "zxcvbnmlkjpoiuyhgtrqfaewds", each represents one alphabet in order

## Testing 
- Describe your testing procedure and how you determined that the system works correctly. Some projects may not have traditional test cases (e.g. tested the system by human interaction with it), but all projects must be tested.
- First we make test cases testing for each individual functions of the project. For example, test_led_blink.c checks if the board is functioning (toggling the LEDs as expected) after soldering the headers. test_i2c_scan.c is used to check if the given devices work well and can send and receive messages as expected. By checking if the received message matches the sent one, this also checks for correct addressing of pins. Also, test_uart.c is used to verify if the serail communication on GPIO pins are working correctly. In addition, board_a_master.c and board_b_slave.c also test the behaviors of I2C and if the message can be sent and received by it correctly.
- After the encryption and decryption methods are finished. We first test them through compiler to verify that it's correctly encoding and decoding strings. Then we incorporate the code into the functions for Alice (board A) and Bob (board B) and test the whole process, which is demonstrated in the secure I2C video in System Overview section.

## Potential Flaws
- We got 2 6.8kohm resistors and 2 10kohm resistors. Since the recommended standard resistor value for I2C channel is around 4.2kohms, we used the 6.8kohm pair in the circuit for unity, which might cause a problem in the performance, especially at higher speeds. During our testing phase, we observe that sometimes Bob receives the message and shows it on the screen, but Eve doesn't intercept it. This happens occasionally, so we believe it's not a problem with our code, but a problem with the boards' connection.
- For now we are using a given shared key based on substitution cipher, if Eve is smarter and Alice sends a long sentence for once, it might be possible for Eve to decode the message using brute force. Once Eve successfully decodes the message once, it would have access to the decryption key just like Bob, and the encoded message would be transparent to Eve again.
- The current encryption key only applies to the 26 alphabets, both uppercase and lowercase. However, for some other characters, such as numbers and spaces, they are the same before and after encryption, which might be a clue to Eve.

## Resources
- https://github.com/orgs/community/discussions/180577
  - To add png/mp4 files on Github Page

## Work Distribution
- This proect was developped collaboratively. Daniel worked on setting up the I2C circuit, coding message transmission between Alice, Bob, and Eve, and debugging UART and displaying messages, and Irene worked on making webpage, adding encryption methods, and soldering the physicla components. We mainly communicate through messages and met on Saturday afternoon to discuss our current plan and what to do next. When we worked individually, after we finished part of the code or the webpage section, we would commit changes to these shared files. When we encountered difficulties, we would try to first solve it by ourselves, then message to see of the other one has any idea, and ask TA if still can't figure it out. One of the biggest difficulties is that since we don't meet very often, it's hard to keep track of the process, but we tried our best to stay updated through messages. Since it's near the end of semester and we're both busy with homeworks and early finals, we try to do the work over weekends and start again as soon as the other assignments are done. 
