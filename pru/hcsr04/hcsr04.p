.origin 0
.entrypoint START

#define TRIGGER_SIGNAL_US 10
#define INS_PER_US 200
#define INS_PER_LOOP 2
#define TRIGGER_DELAY (TRIGGER_SIGNAL_US * INS_PER_US) / INS_PER_LOOP
#define DELAY_1MS (1000 * INS_PER_US) / INS_PER_LOOP
#define PRU0_R31_VEC_VALID 32
#define PRU_EVTOUT_0 3

START:
    MOV r0, TRIGGER_DELAY   //load trigger signal duration
    SET r30.t5              //set trigger pin high

TRIGGERING:                 //delay 10us
    SUB r0, r0, 1
    QBNE TRIGGERING, r0, 0
    CLR r30.t5              //after 10us set trigger pin low

//measure echo signal duration
    MOV r1, 0               //will stor echo signal duration
    WBS r31.t3              //wait till echo pin goes high

ECHOING:
    ADD r1, r1, 1
    QBBS ECHOING, r31.t3    //jump to ECHOING if echo pin is high

    //store echo signal duration in memory
    //duration of this signal is in 10ns units
    MOV r0, 0x00000000      //address in memory where data goes to
    SBBO r1, r0, 0, 4       //store data in memory

END:
    MOV R31.b0, PRU0_R31_VEC_VALID | PRU_EVTOUT_0
    HALT
