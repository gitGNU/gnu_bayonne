// Copyright (C) 2008-2009 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <bayonne.h>

#define MAX_DIGITS      48
#define MAX_NAME_LEN    64

#define DEBUG1  shell::DEBUG0
#define DEBUG2  (shell::loglevel_t(((unsigned)shell::DEBUG0 + 1)))
#define DEBUG3  (shell::loglevel_t(((unsigned)shell::DEBUG0 + 2)))

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

class Timeslot;

typedef uint16_t    keymask_t;
typedef uint32_t    sigmask_t;

/**
 * Bayonne driver state tables.  When a script command requires the driver to
 * be in a specific processing state to perform a blocking operation, it sends
 * a step() request to the driver.  Different step() requests put the driver
 * into different processing states, which then fall back to scripting when
 * the state completes.
 */
typedef enum {
    STEP_HANGUP = 0,
    STEP_SLEEP,
    STEP_ACCEPT,
    STEP_REJECT,
    STEP_ANSWER,
    STEP_COLLECT,
    STEP_PLAY,
    STEP_PLAYWAIT,
    STEP_RECORD,
    STEP_TONE,
    STEP_DIALXFER,
    STEP_SOFTDIAL,
    STEP_FLASH,
    STEP_JOIN,
    STEP_LISTEN,
    STEP_DETECT,
    STEP_TRANSFER,
    STEP_INTERCOM,
    STEP_PICKUP,
    STEP_THREAD,
    STEP_SENDFAX,
    STEP_RECVFAX,
    STEP_EXIT = STEP_HANGUP
} tsstep_t;

/**
  * Signaled interpreter events.  These can be masked and accessed
  * through ^xxx handlers in the scripting language.
  */
typedef enum {
    SIGNAL_EXIT = 0,
    SIGNAL_HANGUP = SIGNAL_EXIT,
    SIGNAL_ERROR,
    SIGNAL_TIMEOUT,
    SIGNAL_DTMF,

    SIGNAL_0,
    SIGNAL_1,
    SIGNAL_2,
    SIGNAL_3,
    SIGNAL_4,
    SIGNAL_5,
    SIGNAL_6,
    SIGNAL_7,
    SIGNAL_8,
    SIGNAL_9,
    SIGNAL_STAR,
    SIGNAL_POUND,
    SIGNAL_A,
    SIGNAL_OVERRIDE = SIGNAL_A,
    SIGNAL_B,
    SIGNAL_FLASH = SIGNAL_B,
    SIGNAL_C,
    SIGNAL_IMMEDIATE = SIGNAL_C,
    SIGNAL_D,
    SIGNAL_PRIORITY = SIGNAL_D,

    SIGNAL_SILENCE,
    SIGNAL_BUSY,
    SIGNAL_CANCEL,
    SIGNAL_FAIL = SIGNAL_CANCEL,
    SIGNAL_INVALID = SIGNAL_CANCEL,
    SIGNAL_NOTIFY,

    SIGNAL_NOANSWER,
    SIGNAL_RING,
    SIGNAL_ANSWER = SIGNAL_RING,
    SIGNAL_PICKUP = SIGNAL_RING,
    SIGNAL_TONE,
    SIGNAL_EVENT,

    SIGNAL_TIME,
    SIGNAL_MAXTIME = SIGNAL_TIME,
    SIGNAL_CHILD,
    SIGNAL_ASR
} tssignal_t;

typedef enum {
    ENTER_STATE = 100,
    EXIT_STATE,
    STOP_STATE,
    EXIT_NOTIFY,
    TIMER_EXIRED,
    TIMER_EXIT,
    TIMER_SYNC,
    SERVICE_SUCCESS,
    SERVICE_FAILURE,
    SERVICE_AUTHORIZE,
    SIGNAL_TEXT,
    SEND_MESSAGE,
    JOIN_TIMESLOTS,
    PART_TIMESLOTS,
    NULL_EVENT,
    CHILD_START,
    CHILD_FAIL,
    CHILD_EXIT,
    JOIN_NOTIFY,
    FAX_EVENT,

    SHELL_EXIT = 200,
    SHELL_START,
    SHELL_WAIT,
    START_SCRIPT,
    RING_START,
    RING_REDIRECT,
    STOP_DISCONNECT,
    SYNC_PARENT,

    ASR_START = 300,
    ASR_TEXT,
    ASR_PARTIAL,
    ASR_VOICE,

    START_INCOMING = RING_START,
    START_OUTGOING = START_SCRIPT,

    MAKE_TEST = 400,
    MAKE_BUSY,
    MAKE_IDLE,
    MAKE_STEP,
    MAKE_STANDBY,
    TEST_IDLE,
    TEST_FAILURE,

    LINE_WINK = 500,
    RINGING_ON,
    RINGING_OFF,
    RINGING_DID,
    ON_HOOK,
    OFF_HOOK,
    CALLER_ID,

    CALL_DETECT = 600,
    CALL_CONNECT,
    CALL_RELEASE,
    CALL_ACCEPT,
    CALL_ANSWERED,
    CALL_HOLD,
    CALL_NOHOLD,
    CALL_DIGITS,
    CALL_OFFER,
    CALL_ANI,
    CALL_ACTIVE,
    CALL_NOACTIVE,
    CALL_BILLING,
    CALL_RESTART,
    CALL_SETSTATE,
    CALL_FAILURE,
    CALL_ALERTING,
    CALL_INFO,
    CALL_BUSY,
    CALL_DIVERT,
    CALL_FACILITY,
    CALL_FRAME,
    CALL_NOTIFY,
    CALL_NSI,
    CALL_RINGING,
    CALL_DISCONNECT,

    DEVICE_OPEN = 700,
    DEVICE_CLOSE,
    DEVICE_BLOCKED,
    DEVICE_UNBLOCKED,

    AUDIO_IDLE = 800,
    AUDIO_BUFFER,
    AUDIO_START,
    AUDIO_STOP,
    INPUT_PENDING,
    OUTPUT_PENDING,
    DTMF_KEYDOWN,
    DTMF_KEYUP,
    VOX_DETECT,
    VOX_SILENCE,
    TONE_START,
    TONE_STOP,
    TONE_IDLE,
    DSP_READY,
    CPA_DIALTONE,
    CPA_BUSYTONE,
    CPA_RINGING,
    CPA_RINGBACK = CPA_RINGING,
    CPA_INTERCEPT,
    CPA_NODIALTONE,
    CPA_NORINGBACK,
    CPA_NOANSWER,
    CPA_CONNECT,
    CPA_FAILURE,
    CPA_GRUNT,
    CPA_REORDER,
    CPA_STOPPED,

    DRIVER_SPECIFIC = 900
} tsevent_t;

/**
 * Speed adjustments for audio.  For timeslot drivers which can do this.
 */
typedef enum {
    SPEED_FAST,
    SPEED_SLOW,
    SPEED_NORMAL
} tsspeed_t;

typedef enum {
    PULSE_DIALER,
    DTMF_DIALER,
    MF_DIALER
} tsdialer_t;

/**
 * This is most often associated with drivers that dynamically allocate dsp
 * resources as needed on demand.  This is used to indicate which dsp resource
 * has currently been allocated for the timeslot.
 */
typedef enum {
    DSP_MODE_INACTIVE = 0,  // dsp is idle
    DSP_MODE_VOICE,     // standard voice processing
    DSP_MODE_CALLERID,  // caller id support
    DSP_MODE_DATA,      // fsk modem mode
    DSP_MODE_FAX,       // fax support
    DSP_MODE_TDM,       // TDM bus with echo cancellation
    DSP_MODE_RTP,       // VoIP full duplex
    DSP_MODE_DUPLEX,    // mixed play/record
    DSP_MODE_JOIN,      // support of joined channels
    DSP_MODE_CONF,      // in conference
    DSP_MODE_TONE       // tone processing
} dspmode_t;

/**
 * This is a special "data" block that is embedded in each channel.  The
 * content of this data block depends on which state the driver timeslot is
 * currently processing (stepped) into.  Hence, different driver states each
 * use different representions of this state data.
 */
typedef union {
    struct {
        unsigned rings;             // rings to answer on
        timeout_t timeout;          // maximum wait time
        const char *transfer;       // id to transfer to if transfering answer
        Timeslot *intercom;         // intercom for transfering answer
        const char *station;        // if answer to fax tone, station id
        const char *fax;            // fax branch script vector
    }   answer;

    struct {
        char pathname[256];
        const char *station;        // fax station id
    }   fax;

    struct {
        union {
            Phrasebook::rule_t rule;
            char path[1024];
        } list;
        Env::pathinfo_t pathinfo;
        unsigned index;             // index into parsed rule list
        unsigned long offset;
        unsigned long limit;
        keymask_t terminate;    // termination keys...
        timeout_t timeout, maxtime;
        unsigned repeat, volume;
        float gain, pitch;
        tsspeed_t speed;
        const char *text;
    } play;

    struct {
        Env::pathinfo_t pathinfo;
        const char *name, *save;
        const char *annotation, *encoding, *text;
        timeout_t timeout;          // max record time
        unsigned long offset;
        keymask_t terminate;
        unsigned long silence, trim, minsize;
        unsigned volume;
        float gain;
        short frames;
        bool append;
        bool info;
        char filepath[128];
        char savepath[128];
    } record;

    struct {
        char digits[MAX_DIGITS + 1];
        char *digit;                // pointer into digits
        const char *callingdigit;
        bool exit;                  // hangup on completion flag
        tsdialer_t dialer;
        timeout_t interdigit;
        timeout_t digittimer;       // for pulse dialing
        timeout_t timeout;
        timeout_t offhook, onhook;  // flash hook timing
        unsigned pulsecount;
    } dialxfer;

    struct {
        timeout_t timeout, first;
        unsigned count;
        keymask_t terminate;
        keymask_t ignore;
        void *map;
        const char *var;
    } collect;

    struct {
        timeout_t wakeup;
        unsigned rings, loops;
        const char *var;
    } sleep;

} tsdata_t;

typedef struct {
    tsevent_t id;

    union {
        struct {
            unsigned digit: 4;          // dtmf digit recieved
            unsigned duration: 12;      // duration of digit
            unsigned e1: 8;             // energy level of tones
            unsigned e2: 8;
        } dtmf;

        struct {
            unsigned tone: 8;
            unsigned energy: 8;         // energy level of tone
            unsigned duration: 16;      // duration of tone
            char *name;                 // name of tone
        } tone;

        struct {
            unsigned digit:  4;         // id if ring cadence supported
            unsigned duration: 24;      // duration of ring
        } ring;

        tsevent_t reason;
        unsigned span, card, tid;
        bool ok;
        int status;
        pid_t pid;
        fd_t fd;
        tsstep_t step;
        char dn[8];
        const char *error;
        dspmode_t dsp;
    } parm;
} Event;

class Message : public LinkedObject
{
protected:
    Event event;
    Timeslot *ts;

public:
    Message(Timeslot *timeslot, Event *msg);

    static void deliver(void);
    static void start(int priority);
    static void stop(void);
};

class Group : public LinkedObject
{
protected:
    friend class Driver;

    keydata *keys;
    const char *id;

    unsigned tsFirst, tsCount;
    unsigned span;

    Group(const char *name);
    Group(unsigned count);

public:
    inline bool isSpan(void)
        {return span != ((unsigned)(-1));};

    inline const char *get(const char *id)
        {return (keys == NULL) ? NULL : keys->get(id);};

    inline const char *get(void)
        {return id;};
};

class Driver
{
private:
    friend class Group;

    static Driver *instance;

protected:
    bool detached;
    const char *name;
    unsigned tsCount, tsUsed, tsSpan;
    Timeslot **tsIndex;
    char *status;
    keydata *keys;

    volatile unsigned active, down;

    Driver(const char *name, const char *registry = "groups");

    virtual int start(void);
    virtual void stop(void);

    keydata *getKeys(const char *groupid);

public:
    inline static Driver *getDriver(void)
        {return instance;};

    inline static bool getDetached(void)
        {return instance->detached;};

    inline static const char *getName(void)
        {return instance->name;};

    inline static const char *getStatus(void)
        {return instance->status;};

    inline static unsigned getCount(void)
        {return instance->tsCount;};

    inline static unsigned getUsed(void)
        {return instance->tsUsed;};

    static Group *getGroup(const char *id);

    static Group *getSpan(unsigned id);

    static Group *getSpan(const char *id);

    static Timeslot *access(unsigned index);

    static Timeslot *request(Group *group = NULL);

    static void release(Timeslot *timeslot);

    static keydata *getPaths(void);

    static int startup(void);

    static void shutdown(void);
};

class Timeslot : protected Script::interp, protected Mutex
{
protected:
    friend class Message;
    friend class Driver;

    Group *incoming;        // incoming group to answer as...
    Group *span;

    Timeslot(unsigned port, Group *group = NULL);

public:
    virtual unsigned long getIdle(void);        // idle time

    virtual bool post(Event *event);

    bool send(Event *event);

    inline void notify(Event *event)
        {new Message(this, event);};
};

class Schedule : public LinkedObject
{
protected:
    void init(void);

public:
    const char *group, *event, *script;
    unsigned year, month, start, end;
    bool dow[8];

    Schedule();

    void select(LinkedObject *list);
};

END_NAMESPACE

