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

/**
 * GNU Bayonne library namespace.  This gives the server it's own
 * private namespace for drivers, plugins, etc, and defines the script engine.
 * @file bayonne.h
 */

#ifndef _BAYONNE_H_
#define _BAYONNE_H_

#define BAYONNE_NAMESPACE   bayonne
#define NAMESPACE_BAYONNE   namespace bayonne {

#ifndef UCOMMON_UCOMMON_H_
#include <ucommon/ucommon.h>
#endif

/**
 * Common namespace for bayonne server.
 * We use a bayonne specific namespace to easily seperate bayonne
 * interfaces from other parts of GNU Telephony.  This namespace
 * is controlled by the namespace macros (BAYONNE_NAMESPACE and
 * NAMESPACE_BAYONNE) and are used in place of direct namespace
 * declarations to make parsing of tab levels simpler and to allow easy
 * changes to the namespace name later if needed.
 * @namespace bayonne
 */

/**
 * Example of creating a script interpreter.
 * @example script.cpp
 */

NAMESPACE_BAYONNE
using namespace UCOMMON_NAMESPACE;

/**
 * Compiled script container.  This class holds the image of a
 * reference counted instance of a compiled script.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Script : public CountedObject, public memalloc
{
public:
    class interp;
    class header;
    class checks;

    /**
     * A type for runtime script method invokation.
     */
    typedef bool (Script::interp::*method_t)(void);

    /**
     * Basic compiled statement.  This is a linked list of statement
     * lines, with an array of parsed statement arguments.  An optional
     * dsp resource mask value can be assigned by a check method on a
     * per-line basis as well as at closure time for a script.
     */
    typedef struct line {
        struct line *next;
        union {
            const char *cmd;
            header *sub;
        };
        char **argv;
        unsigned short loop, argc;
        unsigned lnum, mask;
        method_t method;
    } line_t;

    /**
     * A type for compile-time command verification method invokation.
     */
    typedef const char *(*check_t)(Script *img, Script::header *scr, Script::line_t *line);

    /**
     * A structure to introduce new core commands to the runtime engine.
     * This is typically passed to assign as an array.  Entries with no
     * runtime method (method = NULL) are only processed at compile-time.
     */
    typedef struct keyword
    {
    public:
        const char *name;           /**< name of command */
        method_t method;    /**< runtime method or NULL if c-t only */
        check_t check;              /**< compile-time check routine */
        struct keyword *next;       /**< linked list set by assign() */
    } keyword_t;

    /**
     * Contains defined variables found by scope when strict is used.
     * This is used as a per-script scope list of defined variable objects.
     * This is constructed during check routines and used to validate compile-
     * time symbol references per-argument per-statement.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT strict : public LinkedObject
    {
    public:
        const char *id;

    public:
        static bool find(Script *img, header *scr, const char *id);
        static void createVar(Script *img, header *scr, const char *id);
        static void createSym(Script *img, header *scr, const char *id);
        static void createAny(Script *img, header *scr, const char *id);
        static void createGlobal(Script *img, const char *id);

        void put(FILE *fp, const char *header);
    };

    /**
     * Contains instance of a runtime symbol.  Symbols may be read-only or
     * read-write.  Symbols that refer to data stored elsewhere are
     * considered references, and references are used to pass arguments into
     * defined functions.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT symbol : public LinkedObject
    {
    public:
        const char *name;   /**< name of symbol */
        char *data;         /**< content of symbol */
        unsigned size;      /**< size of data buffer or 0 if const */
        header *scope;      /**< scope of symbol definition */
    };

    /**
     * An event block for a script.  Each script can have one or more named
     * event chains.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT event : public LinkedObject
    {
    public:
        line_t *first;
        const char *name;
    };

    /**
     * Convenience typedef to allow use of event name.
     */
    typedef event event_t;

    /**
     * Header describes a script section.  A section might be a named
     * label or a defined function.  Each section has a list of one or
     * more statement lines, and can have events.  Defined sections may
     * also track per-script scoped variables in strict compile mode.
     * Script headers may also have resource masks assigned at closure.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT header : public LinkedObject
    {
    public:
        LinkedObject *scoped;   /**< scoped symbol defs */
        LinkedObject *events;   /**< named events */
        LinkedObject *methods;  /**< named members */
        line_t *first;          /**< first line of section or define */
        const char *file;       /**< filename of script */
        const char *name;       /**< name of script section or define */
        unsigned resmask;       /**< post-compile processing resource mask */

        /**
         * Used to set linked list linkage.
         * @param scr header to link with.
         */
        inline void link(header *scr)
            {next = scr;};
    };

    /**
     * A container class for compile-time check methods.  Check methods
     * are defined for each script statement.  A derived application
     * server would use this as a base class for the check methods of
     * it's own extensions.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT checks
    {
    public:
        static bool isValue(const char *text);
        static bool isText(const char *text);

        static const char *chkPush(Script *img, header *scr, line_t *line);
        static const char *chkApply(Script *img, header *scr, line_t *line);
        static const char *chkIgnore(Script *img, header *scr, line_t *line);
        static const char *chkNop(Script *img, header *scr, line_t *line);
        static const char *chkExit(Script *img, header *scr, line_t *line);
        static const char *chkVar(Script *img, header *scr, line_t *line);
        static const char *chkConst(Script *img, header *scr, line_t *line);
        static const char *chkSet(Script *img, header *scr, line_t *line);
        static const char *chkClear(Script *img, header *scr, line_t *line);
        static const char *chkError(Script *img, header *scr, line_t *line);
        static const char *chkPack(Script *img, header *scr, line_t *line);
        static const char *chkExpand(Script *img, header *scr, line_t *line);
        static const char *chkGosub(Script *img, header *src, line_t *line);
        static const char *chkGoto(Script *img, header *scr, line_t *line);
        static const char *chkDo(Script *img, header *scr, line_t *line);
        static const char *chkUntil(Script *img, header *scr, line_t *line);
        static const char *chkWhile(Script *ing, header *scr, line_t *line);
        static const char *chkConditional(Script *img, header *scr, line_t *line);
        static const char *chkContinue(Script *img, header *scr, line_t *line);
        static const char *chkBreak(Script *img, header *scr, line_t *line);
        static const char *chkLoop(Script *img, header *scr, line_t *line);
        static const char *chkPrevious(Script *img, header *scr, line_t *line);
        static const char *chkIndex(Script *img, header *scr, line_t *line);
        static const char *chkForeach(Script *img, header *scr, line_t *line);
        static const char *chkCase(Script *img, header *scr, line_t *line);
        static const char *chkEndcase(Script *img, header *scr, line_t *line);
        static const char *chkOtherwise(Script *img, header *scr, line_t *line);
        static const char *chkIf(Script *img, header *scr, line_t *line);
        static const char *chkElif(Script *img, header *scr, line_t *line);
        static const char *chkElse(Script *img, header *scr, line_t *line);
        static const char *chkEndif(Script *img, header *scr, line_t *line);
        static const char *chkDefine(Script *img, header *scr, line_t *line);
        static const char *chkInvoke(Script *img, header *scr, line_t *line);
        static const char *chkWhen(Script *img, header *scr, line_t *line);
        static const char *chkStrict(Script *img, header *scr, line_t *line);
        static const char *chkExpr(Script *img, header *scr, line_t *line);
        static const char *chkRef(Script *img, header *scr, line_t *line);
        static const char *chkIgnmask(Script *img, header *scr, line_t *line);
    };

    /**
     * Runtime stack for each interpreter instance.  This is used to
     * manage loop and case blocks, as well as subroutine calls.
     */
    typedef struct {
        header *scr;            /**< executing script for stack */
        header *scope;          /**< effective symbol scope */
        event_t *event;         /**< so we don't redo our event */
        line_t *line;           /**< executing line at stack level */
        line_t *ignore;         /**< ignored events */
        unsigned short index;   /**< index marker for loops */
        unsigned short base;    /**< base stack of "@section" */
        unsigned short resmask; /**< effective dsp resource mask */
    } stack_t;

    /**
     * An instance of the runtime interpreter.  Some application servers,
     * like GNU Bayonne, may create an interpreter instance for each
     * telephone call session.  All runtime execution happens through
     * the interpreter class, which executes compiled script images.  This
     * is commonly used as a base class for runtime methods and application
     * specific interpreters.  All interpreter instance data and symbols
     * are allocated off the interpreter instance private heap.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT interp : protected memalloc
    {
    public:
        typedef char num_t[16];

        interp();
        virtual ~interp();

        /**
         * Step through an instance of the interpreter.  This can step
         * through multiple lines at once, depending on Script::stepping.
         * @return true if still running, false if exited.
         */
        bool step(void);

        /**
         * Attach a compiled image to the interpreter and start.  Even
         * when different entry points are called, the initialization
         * block is always called.
         * @param image to attach (ref count).
         * @param entry point, NULL for "@main"
         * @return true ifi successful, false if failed to find entry.
         */
        bool attach(Script *image, const char *entry = NULL);

        /**
         * Cleanup after interpreter run.  Releases reference to image.
         */
        void detach(void);

        /**
         * Used to initialize and purge the interpreter between runs.
         * This is used especially if the same interpreter object is kept
         * in memory and directly re-used for multiple executions.
         */
        void initialize(void);

        /**
         * Invoke runtime interpreter error handling.
         * @param text to post into %error symbol.
         */
        bool error(const char *text);

        /**
         * Get current dsp resource mask.
         * @return resource mask.
         */
        unsigned getResource(void);

        /**
         * Get effective filename of base.
         * @return filename.
         */
        const char *getFilename(void);

    protected:
        symbol *find(const char *id);
        void skip(void);
        void push(void);
        bool trylabel(const char *id);
        bool tryexit(void);
        void pullScope(void);
        void pullBase(void);
        void pullLoop(void);
        bool pop(void);
        void setStack(header *scr, event *ev = NULL);

        /**
         * Try to branch to a named event handler.  If successful, the
         * interpreter transfers control to the start of the handler
         * for the next step.
         * @param name of event to request.
         * @return true if found.
         */
        bool scriptEvent(const char *name);

        /**
         * Search for an event object in the method table.
         * @param name to search for.
         * @return method if found.
         */
        event *scriptMethod(const char *name);

        stack_t *stack;
        object_pointer<Script> image;
        LinkedObject **syms;
        unsigned frame;

        char *getTemp(void);
        bool setConst(const char *id, const char *value);
        symbol *createSymbol(const char *id);
        symbol *getVar(const char *id, const char *value = NULL);
        const char *getValue(void);
        const char *getContent(void);
        const char *getContent(const char *text);
        const char *getKeyword(const char *id);
        method_t getLooping(void);
        bool isConditional(unsigned index);
        void setRef(header *scope, const char *id, char *data, unsigned size);
        void getParams(header *scope, line_t *line);
        void startScript(header *scr);

        virtual unsigned getTypesize(const char *type_id);
        virtual const char *getTypeinit(const char *type_id);
        virtual const char *getFormat(symbol *sym, const char *id, char *temp);
        virtual bool getCondition(const char *test, const char *value);
        const char *getIndex(void);

    private:
        bool getExpression(unsigned index);

        char *errmsg;
        char *temps[3];
        unsigned tempindex;
    };

    /**
     * A class to collect compile-time errors.  These are collected as the
     * script is compiled, and can then be examined.  This allows one to
     * push errors into an alternate logging facility.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT error : public OrderedObject
    {
    private:
        friend class Script;
        error(Script *img, unsigned line, const char *str);

    public:
        const char *filename;
        char *errmsg;
        unsigned errline;
    };

    /**
     * Runtime methods collection class.  This is used to collect the
     * runtime implimentation of each script command.  Often derived
     * application servers will use sideway inheritance of something
     * derived from interp to collect implimentations for convenience.
     * @author David Sugar <dyfet@gnutelephony.org>
     */
    class __EXPORT methods : public interp
    {
    public:
        bool scrPush(void);
        bool scrApply(void);
        bool scrExpr(void);
        bool scrVar(void);
        bool scrSet(void);
        bool scrAdd(void);
        bool scrClear(void);
        bool scrConst(void);
        bool scrPause(void);
        bool scrNop(void);
        bool scrPack(void);
        bool scrExpand(void);
        bool scrExit(void);
        bool scrReturn(void);
        bool scrError(void);
        bool scrRestart(void);
        bool scrGosub(void);
        bool scrGoto(void);
        bool scrDo(void);
        bool scrLoop(void);
        bool scrUntil(void);
        bool scrWhile(void);
        bool scrBreak(void);
        bool scrContinue(void);
        bool scrForeach(void);
        bool scrPrevious(void);
        bool scrRepeat(void);
        bool scrIndex(void);
        bool scrCase(void);
        bool scrEndcase(void);
        bool scrOtherwise(void);
        bool scrIf(void);
        bool scrElif(void);
        bool scrElse(void);
        bool scrEndif(void);
        bool scrDefine(void);
        bool scrInvoke(void);
        bool scrWhen(void);
        bool scrRef(void);
        bool scrIgnore(void);
    };

    ~Script();

    static unsigned stepping;   /**< default stepping increment */
    static unsigned indexing;   /**< default symbol indexing */
    static size_t paging;       /**< default heap paging */
    static unsigned sizing;     /**< default symbol size */
    static unsigned stacking;   /**< stack frames in script runtime */
    static unsigned decimals;   /**< default decimal places */

    LinkedObject *scheduler;    /**< scheduler list */

    /**
     * Compiled a file into an existing image.  A shared config script can be
     * used that holds common definitions.  Multiple script files can also be
     * merged together into a final image.
     * @param merge with prior compiled script.
     * @param filename to compile.
     * @param config image of script with common definitions.
     * @return compiled script object if successful.
     */
    static Script *compile(Script *merge, const char *filename, Script *config = NULL);

    /**
     * Assign new keywords from extensions and derived service.  Must
     * be called before any use.
     * @param list of keywords to add to engine.
     */
    static void assign(keyword_t *list);

    /**
     * Find a keyword from internal command table.  This includes the
     * core runtime engine keywords set through init() and any derived
     * ones added through assign.
     * @param id of command to find.
     * @return keyword object for the command or NULL.
     */
    static keyword_t *find(const char *id);

    /**
     * Initialize entire script engine.  Must be called first and once.
     */
    static void init(void);

    static unsigned offset(const char *list, unsigned index);
    static void copy(const char *list, char *item, unsigned size);
    static unsigned count(const char *list);
    static const char *get(const char *list, unsigned offset);
    static char *get(char *list, unsigned offset);
    static header *find(Script *img, const char *id);
    static bool isEvent(header *scr, const char *id);

    header *first;
    LinkedObject **scripts;

    bool push(line_t *line);
    method_t pull(void);
    method_t looping(void);

    inline unsigned getErrors(void)
        {return errors;};

    inline LinkedObject *getListing(void)
        {return errlist.begin();};

    inline const char *getFilename(void)
        {return filename;};

    inline bool isStrict(void)
        {return global != NULL;};

    inline unsigned getLines(void)
        {return lines;};

private:
    friend class strict;
    friend class checks;
    friend class error;
    friend class interp;
    friend class methods;

    Script();

    void errlog(unsigned line, const char *fmt, ...);

    unsigned long instance;
    unsigned errors;
    unsigned loop;
    unsigned lines;
    bool thencheck;
    line_t **stack;
    LinkedObject *global;
    OrderedIndex errlist;
    object_pointer<Script> shared;
    const char *filename;
    unsigned serial;
    LinkedObject *headers;
};

/**
 * Generic audio class to hold master data types and various useful
 * class encapsulated friend functions as per GNU Common C++ 2 coding
 * standard.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Master audio class.
 */
class __EXPORT Audio
{
public:
    typedef int16_t snd16_t;
    typedef int32_t snd32_t;
    typedef int16_t level_t;
    typedef int16_t sample_t;
    typedef int16_t *linear_t;

#if _MSC_VER > 1400        // windows broken dll linkage issue...
    const static unsigned ndata = (-1);
#else
    const static unsigned ndata;
#endif

    typedef struct {
    float v2;
        float v3;
        float fac;
    } goertzel_state_t;

    typedef struct {
        int hit1;
        int hit2;
        int hit3;
        int hit4;
        int mhit;

        goertzel_state_t row_out[4];
        goertzel_state_t col_out[4];
        goertzel_state_t row_out2nd[4];
        goertzel_state_t col_out2nd[4];
        goertzel_state_t fax_tone;
        goertzel_state_t fax_tone2nd;
        float energy;

        int current_sample;
        char digits[129];
        int current_digits;
        int detected_digits;
        int lost_digits;
        int digit_hits[16];
        int fax_hits;
    } dtmf_detect_state_t;

    typedef struct {
        float fac;
    } tone_detection_descriptor_t;

    typedef unsigned char *encoded_t;

    /**
     * Audio encoding rate, samples per second.
     */
    enum    Rate {
        rateUnknown = 0,
        rate6khz = 6000,
        rate8khz = 8000,
        rate11khz = 11025,
        rate16khz = 16000,
        rate22khz = 22050,
        rate32khz = 32000,
        rate44khz = 44100
    };

    typedef enum Rate rate_t;

    /**
     * File processing mode, whether to skip missing files, etc.
     */
    enum    Mode {
        modeRead,
        modeReadAny,
        modeReadOne,
        modeWrite,
        modeCache,
        modeInfo,
        modeFeed,

        modeAppend, // app specific placeholders...
        modeCreate
    };

    typedef enum Mode mode_t;

    /**
     * Audio encoding formats.
     */
    enum Encoding {
        unknownEncoding = 0,
        g721ADPCM,
        g722Audio,
        g722_7bit,
        g722_6bit,
        g723_2bit,
        g723_3bit,
        g723_5bit,
        gsmVoice,
        msgsmVoice,
        mulawAudio,
        alawAudio,
        mp1Audio,
        mp2Audio,
        mp3Audio,
        okiADPCM,
        voxADPCM,
        sx73Voice,
        sx96Voice,

        // Please keep the PCM types at the end of the list -
        // see the "is this PCM or not?" code in
        // AudioFile::close for why.
        cdaStereo,
        cdaMono,
        pcm8Stereo,
        pcm8Mono,
        pcm16Stereo,
        pcm16Mono,
        pcm32Stereo,
        pcm32Mono,

        // speex codecs
        speexVoice, // narrow band
        speexAudio,

        g729Audio,
        ilbcAudio,
        speexUltra,

        speexNarrow = speexVoice,
        speexWide = speexAudio,
        g723_4bit = g721ADPCM
    };
    typedef enum Encoding encoding_t;

    /**
     * Audio container file format.
     */
    enum Format {
        raw,
        snd,
        riff,
        wave
    };
    typedef enum Format format_t;

    /**
     * Audio error conditions.
     */
    enum Error {
        errSuccess = 0,
        errReadLast,
        errNotOpened,
        errEndOfFile,
        errStartOfFile,
        errRateInvalid,
        errEncodingInvalid,
        errReadInterrupt,
        errWriteInterrupt,
        errReadFailure,
        errWriteFailure,
        errReadIncomplete,
        errWriteIncomplete,
        errRequestInvalid,
        errTOCFailed,
        errStatFailed,
        errInvalidTrack,
        errPlaybackFailed,
        errNotPlaying,
        errNoCodec
    };
    typedef enum Error error_t;

    /**
     * Audio source description.
     */
    class __EXPORT Info
    {
    public:
        format_t format;
        encoding_t encoding;
        unsigned long rate;
        unsigned long bitrate;
        unsigned order;
        unsigned framesize, framecount, headersize, padding;
        timeout_t framing;
        char *annotation;

        Info();
        void clear(void);
        void set(void);
        void setFraming(timeout_t frame);
        void setRate(rate_t rate);
    };

    typedef Info info_t;

    /**
     * Convert dbm power level to integer value (0-32768).
     *
     * @param dbm power level
     * @return integer value.
     */
    static level_t tolevel(float dbm);

    /**
     * Convert integer power levels to dbm.
     *
     * @param power level.
     * @return dbm power level.
     */
    static float todbm(level_t power);

    /**
     * Test for the presense of a specified (indexed) audio device.
     * This is normally used to test for local soundcard access.
     *
     * @param device index or 0 for default audio device.
     * @return true if device exists.
     */
    static bool is_available(unsigned device = 0);

    /**
     * Get the mime descriptive type for a given Audio encoding
     * description, usually retrieved from a newly opened audio file.
     *
     * @param info source description object
     * @return text of mime type to use for this audio source.
     */
    static const char *getMIME(Info &info);

    /**
     * Get the short ascii description used for the given audio
     * encoding type.
     *
     * @param encoding format.
     * @return ascii name of encoding format.
     */
    static const char *getName(encoding_t encoding);

    /**
     * Get the preferred file extension name to use for a given
     * audio encoding type.
     *
     * @param encoding format.
     * @return ascii file extension to use.
     */
    static const char *getExtension(encoding_t encoding);

    /**
     * Get the audio encoding format that is specified by a short
     * ascii name.  This will either accept names like those returned
     * from getName(), or .xxx file extensions, and return the
     * audio encoding type associated with the name or extension.
     *
     * @param name of encoding or file extension.
     * @return audio encoding format.
     * @see #getName
     */
    static encoding_t getEncoding(const char *name);

    /**
     * Get the stereo encoding format associated with the given format.
     *
     * @param encoding format being tested for stereo.
     * @return associated stereo audio encoding format.
     */
    static encoding_t getStereo(encoding_t encoding);

    /**
     * Get the mono encoding format associated with the given format.
     *
     * @param encoding format.
     * @return associated mono audio encoding format.
     */
    static encoding_t getMono(encoding_t encoding);

    /**
     * Test if the audio encoding format is a linear one.
     *
     * @return true if encoding format is linear audio data.
     * @param encoding format.
     */
    static bool is_linear(encoding_t encoding);

    /**
     * Test if the audio encoding format must be packetized (that
     * is, has irregular sized frames) and must be processed
     * only through buffered codecs.
     *
     * @return true if packetized audio.
     * @param encoding format.
     */
    static bool is_buffered(encoding_t encoding);

    /**
     * Test if the audio encoding format is a mono format.
     *
     * @return true if encoding format is mono audio data.
     * @param encoding format.
     */
    static bool is_mono(encoding_t encoding);

    /**
     * Test if the audio encoding format is a stereo format.
     *
     * @return true if encoding format is stereo audio data.
     * @param encoding format.
     */
    static bool is_stereo(encoding_t encoding);

    /**
     * Return default sample rate associated with the specified
     * audio encoding format.
     *
     * @return sample rate for audio data.
     * @param encoding format.
     */
    static rate_t getRate(encoding_t encoding);

    /**
     * Return optional rate setting effect.  Many codecs are
     * fixed rate.
     *
     * @return result rate for audio date.
     * @param encoding format.
     * @param requested rate.
     */
    static rate_t getRate(encoding_t e, rate_t request);

    /**
     * Return frame timing for an audio encoding format.
     *
     * @return frame time to use in milliseconds.
     * @param encoding of frame to get timing segment for.
     * @param timeout of frame time segment to request.
     */
    static timeout_t getFraming(encoding_t encoding, timeout_t timeout = 0);

    /**
     * Return frame time for an audio source description.
     *
     * @return frame time to use in milliseconds.
     * @param info descriptor of frame encoding to get timing segment for.
     * @param timeout of frame time segment to request.
     */
    static timeout_t getFraming(Info &info, timeout_t timeout = 0);

    /**
     * Test if the endian byte order of the encoding format is
     * different from the machine's native byte order.
     *
     * @return true if endian format is different.
     * @param encoding format.
     */
    static bool is_endian(encoding_t encoding);

    /**
     * Test if the endian byte order of the audio source description
     * is different from the machine's native byte order.
     *
     * @return true if endian format is different.
     * @param info source description object.
     */
    static bool is_endian(Info &info);

    /**
     * Optionally swap endian of audio data if the encoding format
     * endian byte order is different from the machine's native endian.
     *
     * @return true if endian format was different.
     * @param encoding format of data.
     * @param buffer of audio data.
     * @param number of audio samples.
     */
    static bool swapEndian(encoding_t encoding, void *buffer, unsigned number);

    /**
     * Optionally swap endian of encoded audio data based on the
     * audio encoding type, and relationship to native byte order.
     *
     * @param info source description of object.
     * @param buffer of audio data.
     * @param number of bytes of audio data.
     */
    static void swapEncoded(Info &info, encoded_t data, size_t bytes);

       /**
     * Optionally swap endian of audio data if the audio source
     * description byte order is different from the machine's native
     * endian byte order.
     *
     * @return true if endian format was different.
     * @param info source description object of data.
     * @param buffer of audio data.
     * @param number of audio samples.
     */
    static bool swapEndian(Info &info, void *buffer, unsigned number);

    /**
     * Get the energey impulse level of a frame of audio data.
     *
     * @return impulse energy level of audio data.
     * @param encoding format of data to examine.
     * @param buffer of audio data to examine.
     * @param number of audio samples to examine.
     */
    static level_t impulse(encoding_t encoding, void *buffer, unsigned number);

    /**
     * Get the energey impulse level of a frame of audio data.
     *
     * @return impulse energy level of audio data.
     * @param info encoding source description object.
     * @param buffer of audio data to examine.
     * @param number of audio samples to examine.
     */
    static level_t impulse(Info &info, void *buffer, unsigned number = 0);

    /**
     * Get the peak (highest energy) level found in a frame of audio
     * data.
     *
     * @return peak energy level found in data.
     * @param encoding format of data.
     * @param buffer of audio data.
     * @param number of samples to examine.
     */
    static level_t peak(encoding_t encoding, void *buffer, unsigned number);

    /**
     * Get the peak (highest energy) level found in a frame of audio
     * data.
     *
     * @return peak energy level found in data.
     * @param info description object of audio data.
     * @param buffer of audio data.
     * @param number of samples to examine.
     */
    static level_t peak(Info &info, void *buffer, unsigned number = 0);

    /**
     * Provide ascii timestamp representation of a timeout value.
     *
     * @param duration timeout value
     * @param address for ascii data.
     * @param size of ascii data.
     */
    static void toTimestamp(timeout_t duration, char *address, size_t size);

    /**
     * Convert ascii timestamp representation to a timeout number.
     *
     * @param timestamp ascii data.
     * @return timeout_t duration from data.
     */
    static timeout_t toTimeout(const char *timestamp);

    /**
     * Returns the number of bytes in a sample frame for the given
     * encoding type, rounded up to the nearest integer.  A frame
     * is defined as the minimum number of bytes necessary to
     * create a point or points in the output waveform for all
     * output channels.  For example, 16-bit mono PCM has a frame
     * size of two (because those two bytes constitute a point in
     * the output waveform).  GSM has it's own definition of a
     * frame which involves decompressing a sequence of bytes to
     * determine the final points on the output waveform.  The
     * minimum number of bytes you can feed to the decompression
     * engine is 32.5 (260 bits), so this function will return 33
     * (because we round up) given an encoding type of GSM.  Other
     * compressed encodings will return similar results.  Be
     * prepared to deal with nonintuitive return values for
     * rare encodings.
     *
     * @param encoding The encoding type to get the frame size for.
     * @param samples Reserved.  Use zero.
     *
     * @return The number of bytes in a frame for the given encoding.
     */
    static int getFrame(encoding_t encoding, int samples = 0);

    /**
     * Returns the number of samples in all channels for a frame
     * in the given encoding.  For example, pcm32Stereo has a
     * frame size of 8 bytes: Note that different codecs have
     * different definitions of a frame - for example, compressed
     * encodings have a rather large frame size relative to the
     * sample size due to the way bytes are fed to the
     * decompression engine.
     *
     * @param encoding The encoding to calculate the frame sample count for.
     * @return samples The number of samples in a frame of the given encoding.
     */
    static int getCount(encoding_t encoding);

    /**
     * Compute byte counts of audio data into number of samples
     * based on the audio encoding format used.
     *
     * @return number of audio samples in specified data.
     * @param encoding format.
     * @param bytes of data.
     */
    static unsigned long toSamples(encoding_t encoding, size_t bytes);

    /**
     * Compute byte counts of audio data into number of samples
     * based on the audio source description used.
     *
     * @return number of audio samples in specified data.
     * @param info encoding source description.
     * @param bytes of data.
     */
    static unsigned long toSamples(Info &info, size_t bytes);

    /**
     * Compute the number of bytes a given number of samples in
     * a given audio encoding will occupy.
     *
     * @return number of bytes samples will occupy.
     * @param info encoding source description.
     * @param number of samples.
     */
    static size_t toBytes(Info &info, unsigned long number);

    /**
     * Compute the number of bytes a given number of samples in
     * a given audio encoding will occupy.
     *
     * @return number of bytes samples will occupy.
     * @param encoding format.
     * @param number of samples.
     */
    static size_t toBytes(encoding_t encoding, unsigned long number);

    /**
     * Fill an audio buffer with "empty" (silent) audio data, based
     * on the audio encoding format.
     *
     * @param address of data to fill.
     * @param number of samples to fill.
     * @param encoding format of data.
     */
    static void fill(unsigned char *address, int number, encoding_t encoding);

    /**
     * Maximum framesize for a given coding that may be needed to
     * store a result.
     *
     * @param info source description object.
     * @return maximum possible frame size to allocate for encoded data.
     */
    static size_t maxFramesize(Info &info);

    /**
    * The AudioResample class is used to manage linear intropolation
    * buffering for rate conversions.
    *
    * @author David Sugar <dyfet@gnutelephony.org>
    * @short linear intropolation and rate conversion.
    */
    class __EXPORT resample
    {
    protected:
        unsigned mfact, dfact, max;
        unsigned gpos, ppos;
        sample_t last;
        linear_t buffer;

    public:
        resample(rate_t mul, rate_t div);
        ~resample();

        size_t process(linear_t from, linear_t to, size_t count);
        size_t estimate(size_t count);
    };
};

/**
 * Bayonne phrasebook for language modules and plugins.  This offers rules
 * parsing and translation for phrasebook systems.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Phrasebook : public LinkedObject
{
public:
    typedef struct
    {
        bool zeroflag;
        unsigned long last;
        unsigned pos;
        unsigned max;
        size_t size;
        char *bp;
        const char *list[1];
    } rule_t;

protected:
    static void _add(const char *text, rule_t *state);

    static void _dup(const char *text, rule_t *state);

    static void _lownumber(int num, rule_t *state);

public:
    Phrasebook(bool primary);

    virtual bool id(const char *lang) = 0;

    virtual const char *path(void);

    virtual void number(const char *text, rule_t *state);

    virtual void order(const char *text, rule_t *state);

    virtual void spell(const char *text, rule_t *state);

    virtual void literal(const char *text, rule_t *state);

    virtual void weekday(const char *text, rule_t *state);

    virtual void date(const char *text, rule_t *state);

    virtual void fulldate(const char *text, rule_t *state);

    virtual void year(const char *text, rule_t *state);

    virtual void time(const char *text, rule_t *state);

    virtual void zero(const char *text, rule_t *state);

    virtual void single(const char *text, rule_t *state);

    virtual void plural(const char *text, rule_t *state);

    virtual void nonzero(const char *text, rule_t *state);

    static Phrasebook *find(const char *locale = NULL);

    static void init(rule_t *state, size_t size);

    static void reset(rule_t *state);

};

/**
 * Core bayonne environment.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Env
{
private:
    static shell_t *sys;
    static bool daemon_flag;
    static bool tool_flag;

public:
    class __LOCAL pathinfo_t
    {
    public:
        Phrasebook *book;
        const char *voices;
        const char *appname;
        const char *extension;

        inline void reset(void)
            {book = NULL; voices = appname = NULL;};

        inline Phrasebook *operator->()
            {return book;};
    };

    static bool init(shell_t *args);

    static void tool(shell_t *args);

    static inline void set(const char *id, const char *value)
        {sys->setsym(id, value);};

    static inline const char *get(const char *id)
        {return sys->getsym(id);};

    static inline const char *env(const char *id)
        {return sys->getsym(id);};

    static inline String path(const char *id)
        {return str(sys->getsym(id));};

    static const char *path(pathinfo_t& pathinfo, const char *path, char *buffer, size_t size, bool write = false);
};

#define DEBUG1  shell::DEBUG0
#define DEBUG2  (shell::loglevel_t(((unsigned)shell::DEBUG0 + 1)))
#define DEBUG3  (shell::loglevel_t(((unsigned)shell::DEBUG0 + 2)))

/**
 * Server control interfaces and functions.  This is an internal management
 * class for the server control fifo and for other server control operations.
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT Control : protected Env
{
public:
    /**
     * Send a printf-style message to the control fifo via the file system.
     * While plugins can also use this to send control messages back into the
     * server, we should create a method that does not require going to the
     * external filesystem to do this.
     * @param format string.
     * @return true if successful.
     */
    static bool send(const char *format, ...) __PRINTF(1, 2);

    /**
     * Used by the server to pull pending fifo requests.
     * @return string of next fifo input.
     */
    static char *receive(void);

    /**
     * Used by the server to send replies back to control requests.
     * @param error string to report or NULL for none.
     */
    static void reply(const char *error = NULL);

    /**
     * Creates the control fifo using server configuration.  This also
     * attaches the shell environment and command line arguments to the
     * current server instance so it can be accessed by other things.
     * @return size of longest control message supported.
     */
    static size_t create(void);

    /**
     * Used by the server to destroy the control fifo.
     */
    static void release(void);

    /**
     * Used to open an output session for returning control data.
     * @param id of output type.
     * @return file handle to write to or NULL on failure.
     */
    static FILE *output(const char *id);

    /**
     * Print/produce log message.
     * @param fmt string.
     */
    static void log(const char *fmt, ...) __PRINTF(1, 2);
};

/**
 * The Tonegen class is used to create a frame of audio encoded single or
 * dualtones.  The frame will be iterated for each request, so a
 * continual tone can be extracted by frame.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short audio tone generator class.
 */
class __EXPORT Tonegen : public Audio, protected Env
{
public:
    typedef struct _tonedef {
        struct _tonedef *next;
        timeout_t duration, silence;
        unsigned count;
        unsigned short f1, f2;
    } def_t;

    typedef struct _tonekey {
        struct _tonekey *next;
        struct _tonedef *first;
        struct _tonedef *last;
        char id[1];
    } key_t;

protected:
    rate_t rate;
    unsigned samples;
    linear_t frame;
    double df1, df2, p1, p2;
    level_t m1, m2;
    bool silencer;
    bool complete;
    key_t *tone;
    def_t *def;
    unsigned remaining, silent, count;
    timeout_t framing;
    level_t level;

    /**
     * Set the frame to silent.
     */
    void silence(void);

    /**
     * Reset the tone generator completely.  Produces silence.,
     */
    void reset(void);

    /**
     * Cleanup for virtual destructors to use.
     */
    void cleanup(void);

    /**
     * Set frame to generate single tone...
     *
     * @param freq of tone.
     * @param level of tone.
     */
    void single(unsigned freq, level_t level);

    /**
     * Set frame to generate dual tone...
     *
     * @param f1 frequency of tone 1
     * @param f2 frequency of tone 2
     * @param l1 level of tone 1
     * @param l2 level of tone 2
     */
    void dual(unsigned f1, unsigned f2, level_t l1, level_t l2);

public:
    /**
     * Construct a silent tone generator of specific frame size.
     *
     * @param duration of frame in milliseconds.
     * @param rate of samples.
     */
    Tonegen(timeout_t duration = 20, rate_t rate = rate8khz);

    /**
     * Create a tone sequencing object for a specific telephony tone
     * key id.
     *
     * @param key for telephony tone.
     * @param level for generated tones.
     * @param frame timing to use in processing.
     */
    Tonegen(key_t *key, level_t level, timeout_t frame = 20);

    /**
     * Construct a dual tone frame generator.
     *
     * @param f1 frequency of tone 1.
     * @param f2 frequency of tone 2.
     * @param l1 level of tone 1.
     * @param l2 level of tone 2.
     * @param duration of frame in milliseconds.
     * @param sample rate being generated.
     */
    Tonegen(unsigned f1, unsigned f2, level_t l1, level_t l2,
        timeout_t duration = 20, rate_t sample = rate8khz);

    /**
     * Construct a single tone frame generator.
     *
     * @param freq of tone.
     * @param level of tone.
     * @param duration of frame in milliseconds.
     * @param sample rate being generated.
     */
    Tonegen(unsigned freq, level_t level, timeout_t duration = 20, rate_t sample = rate8khz);

    virtual ~Tonegen();

    /**
     * Get the sample encoding rate being used for the tone generator
     *
     * @return sample rate in samples per second.
     */
    inline rate_t getRate(void) const
        {return rate;};

    /**
     * Get the frame size for the number of audio samples generated.
     *
     * @return number of samples processed in frame.
     */
    inline size_t getSamples(void) const
        {return samples;};

    /**
     * Test if the tone generator is currently set to silence.
     *
     * @return true if generator set for silence.
     */
    bool is_silent(void);

    /**
     * Iterate the tone frame, and extract linear samples in
     * native frame.  If endian flag passed, then convert for
     * standard endian representation (byte swap) if needed.
     *
     * @return pointer to samples.
     */
    virtual linear_t getFrame(void);

    /**
     * This is used to copy one or more pages of framed audio
     * quickly to an external buffer.
     *
     * @return number of frames copied.
     * @param buffer to copy into.
     * @param number of frames requested.
     */
    unsigned getFrames(linear_t buffer, unsigned number);

    /**
     * See if at end of tone.  This is used for non-continues audio
     * tones, or to detect "break" events.
     *
     * @return true if end of data.
     */
    inline bool is_complete(void)
        {return complete;};

    static bool load(const char *locale = NULL);

    static key_t *find(const char *id, const char *locale = NULL);
};

/**
 * DTMFDialer is used to generate a series of dtmf audio data from a
 * "telephone" number passed as an ASCII string.  Each time getFrame()
 * is called, the next audio frame of dtmf audio will be created
 * and pulled.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Generate DTMF audio
 */
class __EXPORT DTMFDialer : public Tonegen
{
protected:
    unsigned dtmfframes;
    const char *digits;

public:
    /**
     * Generate a dtmf dialer for a specified dialing string.
     *
     * @param digits to generate tone dialing for.
     * @param level for dtmf.
     * @param duration timing for generated audio.
     * @param interdigit timing, should be multiple of frame.
     */
    DTMFDialer(const char *digits, level_t level, timeout_t duration = 20, timeout_t interdigit = 60);

    ~DTMFDialer();

    linear_t getFrame(void);
};

/**
 * MFDialer is used to generate a series of mf audio data from a
 * "telephone" number passed as an ASCII string.  Each time getFrame()
 * is called, the next audio frame of dtmf audio will be created
 * and pulled.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Generate MF audio
 */
class __EXPORT MFDialer : public Tonegen
{
protected:
    unsigned mfframes;
    const char *digits;
    bool kflag;

public:
    /**
     * Generate a mf dialer for a specified dialing string.
     *
     * @param digits to generate tone dialing for.
     * @param level for mf.
     * @param duration timing for generated audio.
     * @param interdigit timing, should be multiple of frame.
     */
    MFDialer(const char *digits, level_t level, timeout_t duration = 20, timeout_t interdigit = 60);

    ~MFDialer();

    linear_t getFrame(void);
};

/**
 * DTMFDetect is used for detecting DTMF tones in a stream of audio.
 * It currently only supports 8000Hz input.
 */
class __EXPORT DTMFDetect : public Audio
{
public:
    DTMFDetect();
    ~DTMFDetect();

    /**
     * This routine is used to push linear audio data into the
     * dtmf tone detection analysizer.  It may be called multiple
     * times and results fetched later.
     *
     * @param buffer of audio data in native machine endian to analysize.
     * @param count of samples to analysize from buffer.
     */
    int putSamples(linear_t buffer, int count);

    /**
     * Copy detected dtmf results into a data buffer.
     *
     * @param data buffer to copy into.
     * @param size of data buffer to copy into.
     */
    int getResult(char *data, int size);

protected:
    void goertzelInit(goertzel_state_t *s, tone_detection_descriptor_t *t);
    void goertzelUpdate(goertzel_state_t *s, sample_t x[], int samples);
    float goertzelResult(goertzel_state_t *s);

private:
    dtmf_detect_state_t *state;
    tone_detection_descriptor_t dtmf_detect_row[4];
    tone_detection_descriptor_t dtmf_detect_col[4];
    tone_detection_descriptor_t dtmf_detect_row_2nd[4];
    tone_detection_descriptor_t dtmf_detect_col_2nd[4];
    tone_detection_descriptor_t fax_detect;
    tone_detection_descriptor_t fax_detect_2nd;
};

/**
 * The codec class is a virtual used for transcoding audio samples between
 * linear frames (or other known format) and an encoded "sample" buffer.
 * This class is only abstract and describes the core interface for
 * loadable codec modules.  This class is normally merged with AudioSample.
 * A derived AudioCodecXXX will typically include a AudioRegisterXXX static
 * class to automatically initialize and register the codec with the codec
 * registry.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short process codec interface.
 */
class __EXPORT AudioCodec : public Audio, public LinkedObject
{
protected:
    encoding_t encoding;
    const char *name;
    info_t info;

    AudioCodec();

    /**
     * often used to create a "new" codec of a subtype based on
     * encoding format, default returns the current codec entity.
     *
     * @return pointer to an active codec or NULL if not found.
     * @param format name from spd.
     */
    virtual AudioCodec *getByFormat(const char *format)
        {return this;};

    /**
     * get a codec by audio source info descriptor.
     *
     * @return pointer to an active codec or NULL if not found.
     * @param info audio source descriptor.
     */
    virtual AudioCodec *getByInfo(info_t& info)
        {return this;};

public:
    /**
     * Base for codecs, create a named coded of a specific encoding.
     *
     * @param name of codec.
     * @param encoding type of codec.
     */
    AudioCodec(const char *name, encoding_t encoding);

    inline const char *getName(void) const
        {return name;};

    inline const char *getDescription(void) const
        {return info.annotation;};

    virtual ~AudioCodec() {};

    /**
     * End use of a requested codec.  If constructed then will be
     * deleted.
     *
     * @param codec pointer to getCodec returned coded pointer.
     */
    static void release(AudioCodec *codec);

    static AudioCodec *begin(void);
    /**
     * Get the codec base class for accessing a specific derived
     * codec identified by encoding type and optional spd info.
     *
     * @return pointer to codec for processing.
     * @param encoding format requested.
     * @param format spd options to pass to codec being created.
     */
    static AudioCodec *get(encoding_t encoding, const char *format = NULL);

    /**
     * Get the codec base class for accessing a specific derived
     * codec identified by audio source descriptor.
     *
     * @return pointer to codec for processing.
     * @param info source descriptor for codec being requested.
     */
    static AudioCodec *get(info_t& info);

    /**
     * Get the impulse energy level of a frame of X samples in
     * the specified codec format.
     *
     * @return average impulse energy of frame (sumnation).
     * @param buffer of encoded samples.
     * @param number of encoded samples.
     */
    virtual level_t impulse(void *buffer, unsigned number = 0);

    /**
     * Get the peak energy level within the frame of X samples.
     *
     * @return peak energy impulse in frame (largest).
     * @param buffer of encoded samples.
     * @param number of encoded samples.
     */
    virtual level_t peak(void *buffer, unsigned number = 0);

    /**
     * Signal if the current audio frame is silent.  This can be
     * deterimed either by an impulse computation, or, in some
     * cases, some codecs may signal and flag silent packets.
     *
     * @return true if silent
     * @param threashold to use if not signaled.
     * @param buffer of encoded samples.
     * @param number of encoded samples.
     */
    virtual bool is_silent(level_t threashold, void *buffer, unsigned number = 0);

    /**
     * Encode a linear sample frame into the codec sample buffer.
     *
     * @return number of bytes written.
     * @param buffer linear sample buffer to use.
     * @param dest buffer to store encoded results.
     * @param number of samples.
     */
    virtual unsigned encode(linear_t buffer, void *dest, unsigned number = 0) = 0;

    /**
     * Encode linear samples buffered into the coded.
     *
     * @return number of bytes written or 0 if incomplete.
     * @param buffer linear samples to post.
     * @param destination of encoded audio.
     * @param number of samples being buffered.
     */
    virtual unsigned encodeBuffered(linear_t Buffer, encoded_t dest, unsigned number);

    /**
     * Decode the sample frame into linear samples.
     *
     * @return number of bytes scanned or returned
     * @param buffer sample buffer to save linear samples into.
     * @param source for encoded data.
     * @param number of samples to extract.
     */
    virtual unsigned decode(linear_t buffer, void *source, unsigned number = 0) = 0;

    /**
     * Buffer and decode data into linear samples.  This is needed
     * for audio formats that have irregular packet sizes.
     *
     * @return number of samples actually decoded.
     * @param destination for decoded data.
     * @param source for encoded data.
     * @param number of bytes being sent.
     */
    virtual unsigned decodeBuffered(linear_t buffer, encoded_t source, unsigned len);

    /**
     * Get estimated data required for buffered operations.
     *
     * @return estimated number of bytes required for decode.
     */
    virtual unsigned getEstimated(void);

    /**
     * get required samples for encoding.
     *
     * @return required number of samples for encoder buffer.
     */
    virtual unsigned getRequired(void);

    /**
     * Get a packet of data rather than decode.  This is tied with
     * getEstimated.
     *
     * @return size of data packet or 0 if not complete.
     * @param destination to save.
     * @param data to push into buffer.
     * @param number of bytes to push.
     */
    virtual unsigned getPacket(encoded_t destination, encoded_t data, unsigned size);

    /**
     * Get an info description for this codec.
     *
     * @return info.
     */
    inline info_t getInfo(void) const
        {return info;};
};

/**
 * A class used to manipulate audio data.  This class provides file
 * level access to audio data stored in different formats.  This class
 * also provides the ability to write audio data into a disk file.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short audio file access.
 */
class __EXPORT AudioFile: public Audio
{
protected:
    char *pathname;
    size_t header;              // offset to start of audio
    size_t minimum;             // minimum sample size required
    size_t iolimit;
    size_t pos, eof;            // saved position mark...
    info_t info;
    mode_t mode;
    fsys_t fs;

    void initialize(void);
    void getWaveFormat(int size);
    AudioCodec *getCodec(void);

    ssize_t read(void *buf, size_t len);
    ssize_t write(void *buf, size_t len);
    ssize_t peek(void *buf, size_t len);
    bool seek(size_t pos);

    virtual const char *getContinuation();

    /**
     * Convert binary 2 byte data stored in the order specified
     * in the source description into a short variable.  This is
     * often used to manipulate header data.  The info type of the
     * object is used to determine the endian format.
     *
     * @return short value.
     * @param data binary 2 byte data pointer.
     */
    unsigned short getShort(unsigned char *data) const;

    /**
     * Save a short as two byte binary data stored in the endian
     * order specified in the source description.  This is often
     * used to manipulate header data.
     *
     * @param data binary 2 byte data pointer.
     * @param value to convert.
     */
    void setShort(unsigned char *data, unsigned short value);

    /**
     * Convert binary 4 byte data stored in the order specified
     * in the source description into a long variable.  This is
     * often used to manipulate header data.  The object's info
     * order is used to determine the endian.
     *
     * @return long value.
     * @param data binary 4 byte data pointer.
     */
    unsigned long getLong(unsigned char *data) const;

    /**
     * Save a long as four byte binary data stored in the endian
     * order specified in the source description.  This is often
     * used to manipulate header data.
     *
     * @param data binary 4 byte data pointer.
     * @param value to convert.
     */
    void setLong(unsigned char *data, unsigned long value);

public:
    /**
     * Construct and open an existing audio file for read/write.
     *
     * @param name of file to open.
     * @param offset to start access.
     */
    AudioFile(const char *name, unsigned long offset = 0);

    /**
     * Create and open a new audio file for writing.
     *
     * @param name of file to create.
     * @param info source description for new file.
     * @param minimum file size to accept at close.
     */
    AudioFile(const char *name, info_t& info, unsigned long minimum = 0);

    /**
     * Construct an audio file without attaching to the filesystem.
     */
    AudioFile();

    virtual ~AudioFile();

    /**
     * Open an audio file and associate it with this object.
     * Called implicitly by the two-argument version of the
     * constructor.
     *
     * @param name of the file to open.  Don't forget to
     * double your backslashes for DOS-style pathnames.
     * @param mode to open file under.
     * @param framing time in milliseconds.
     */
    void open(const char *name, mode_t mode = modeWrite, timeout_t framing = 0);

    /**
     * Create a new audio file and associate it with this object.
     * Called implicitly by the three-argument version of the
     * constructor.
     *
     * @param name The name of the file to open.
     * @param info The type of the audio file to be created.
     * @param framing time in milliseconds.
     */
    void create(const char *name, info_t& info, timeout_t framing = 0);

    /**
     * Returns age since last prior access.  Used for cache
     * computations.
     *
     * @return age in seconds.
     */
    time_t age(void);

    /**
     * Get maximum size of frame buffer for data use.
     *
     * @return max frame size in bytes.
     */
    inline size_t getSize(void)
        {return maxFramesize(info);};

    /**
     * Close an object associated with an open file.  This
     * updates the header metadata with the file length if the
     * file length has changed.
     */
    void close(void);

    /**
     * Clear the AudioFile structure.  Called by
     * AudioFile::close().  Sets all fields to zero and deletes
     * the dynamically allocated memory pointed to by the pathname
     * and info.annotation members.  See AudioFile::initialize()
     * for the dynamic allocation code.
     */
    void clear(void);

    inline int err(void)
        {return fs.err();};

    /**
     * Retrieve bytes from the file into a memory buffer.  This
     * increments the file pointer so subsequent calls read further
     * bytes.  If you want to read a number of samples rather than
     * bytes, use getSamples().
     *
     * @param buffer area to copy the samples to.
     * @param len The number of bytes (not samples) to copy or 0 for frame.
     * @return The number of bytes (not samples) read.  Returns -1
     * if no bytes are read and an error occurs.
     */
    ssize_t getBuffer(encoded_t buffer, size_t len = 0);

    /**
     * Retrieve and convert content to linear encoded audio data
     * from it's original form.
     *
     * @param buffer to copy linear data into.
     * @param request number of linear samples to extract or 0 for frame.
     * @return number of samples retrieved, 0 if no codec or eof.
     */
    unsigned getLinear(linear_t buffer, unsigned request = 0);

    /**
     * Get raw data and assure is in native machine endian.
     *
     * @return data received in buffer.
     * @param data to get.
     * @param size of data to get.
     */
    ssize_t getNative(encoded_t data, size_t size);

    /**
     * Insert bytes into the file from a memory buffer.  This
     * increments the file pointer so subsequent calls append
     * further samples.  If you want to write a number of samples
     * rather than bytes, use putSamples().
     *
     * @param buffer area to append the samples from.
     * @param len The number of bytes (not samples) to append.
     * @return The number of bytes (not samples) read.  Returns -1
     * if an error occurs and no bytes are written.
     */
    ssize_t putBuffer(encoded_t buffer, size_t len = 0);

    /**
     * Puts raw data and does native to refined endian swapping
     * if needed based on encoding type and local machine endian.
     *
     * @param data to put.
     * @param size of data to put.
     * @return number of bytes actually put.
     */
    ssize_t putNative(encoded_t data, size_t size);

    /**
     * Convert and store content from linear encoded audio data
     * to the format of the audio file.
     *
     * @param buffer to copy linear data from.
     * @param request Number of linear samples to save or 0 for frame.
     * @return number of samples saved, 0 if no codec or eof.
     */
    unsigned putLinear(linear_t buffer, unsigned request = 0);

    /**
     * Retrieve samples from the file into a memory buffer.  This
     * increments the file pointer so subsequent calls read
     * further samples.  If a limit has been set using setLimit(),
     * the number of samples read will be truncated to the limit
     * position.  If you want to read a certain number of bytes
     * rather than a certain number of samples, use getBuffer().
     *
     * @param buffer pointer to copy the samples to.
     * @param samples The number of samples to read or 0 for frame.
     * @return 0 if successful, or errno if not.
     */
    int getSamples(void *buffer, unsigned samples = 0);

    /**
     * Insert samples into the file from a memory buffer.  This
     * increments the file pointer so subsequent calls append
     * further samples.  If you want to write a certain number of
     * bytes rather than a certain number of samples, use
     * putBuffer().
     *
     * @param buffer pointer to append the samples from.
     * @param samples The number of samples (not bytes) to append.
     * @return 0 if successful, or errno if not.
     */
    int putSamples(void *buffer, unsigned samples = 0);

    /**
     * Change the file position by skipping a specified number
     * of audio samples of audio data.
     *
     * @return 0 if successful, else errno.
     * @param number of samples to skip.
     */
    int skip(long number);

    /**
     * Seek a file position by sample count.  If no position
     * specified, then seeks to end of file.
     *
     * @return 0 if successful, else error number.
     * @param samples position to seek in file.
     */
    int setPosition(unsigned long samples = ~0l);

    /**
     * Seek a file position by timestamp.  The actual position
     * will be rounded by framing.
     *
     * @return 0 if successful, else errno.
     * @param timestamp position to seek.
     */
    int position(const char *timestamp);

    /**
     * Return the timestamp of the current absolute file position.
     *
     * @param timestamp to save ascii position into.
     * @param size of timestamp buffer.
     */
    void getPosition(char *timestamp, size_t size);

    /**
     * Set the maximum file position for reading and writing of
     * audio data by samples.  If 0, then no limit is set.
     *
     * @param maximum file i/o access size sample position.
     */
    void setLimit(unsigned long maximum = 0l);

    /**
     * Copy the source description of the audio file into the
     * specified object.  Can set error state of object if fails.
     *
     * @param output object to copy source description into.
     */
    inline void getInfo(info_t& output) const
        {output = info;};

    /**
     * Set minimum file size for a created file.  If the file
     * is closed with fewer samples than this, it will also be
     * deleted.
     *
     * @param minimum number of samples for new file.
     * @return 0 if successful, else errno.
     */
    int setMinimum(unsigned long minimum);

    /**
     * Get the current file pointer in bytes relative to the start
     * of the file.  See getPosition() to determine the position
     * relative to the start of the sample buffer.
     *
     * @return The current file pointer in bytes relative to the
     * start of the file.  Returns 0 if the file is not open, is
     * empty, or an error has occured.
     */
    inline size_t getAbsolutePosition(void)
        {return pos;}

    /**
     * Get the current file pointer in samples relative to the
     * start of the sample buffer.  Note that you must multiply
     * this result by the result of a call to
     * toBytes(info.encoding, 1) in order to determine the offset
     * in bytes.
     *
     * @return the current file pointer in samples relative to the
     * start of the sample buffer.  Returns 0 if the file is not
     * open, is empty, or an error has occured.
     */
    unsigned long getPosition(void);

    inline bool operator!() const
        {return !fs;};

    inline operator bool() const
        {return is(fs);};

    inline bool is_open(void) const
        {return is(fs);};

    /**
     * Return audio encoding format for this audio file.
     *
     * @return audio encoding format.
     */
    inline encoding_t getEncoding(void) const
        {return info.encoding;};

    /**
     * Return base file format of containing audio file.
     *
     * @return audio file container format.
     */
    inline format_t getFormat(void) const
        {return info.format;};

    /**
     * Get audio encoding sample rate, in samples per second, for
     * this audio file.
     *
     * @return sample rate.
     */
    inline unsigned getSampleRate(void) const
        {return info.rate;};

    /**
     * Get annotation extracted from header of containing file.
     *
     * @return annotation text if any, else NULL.
     */
    inline char *getAnnotation(void) const
        {return info.annotation;};

    /**
     * Return if the current content is signed or unsigned samples.
     *
     * @return true if signed.
     */
    bool is_signed(void) const;
};

/**
 * AudioStream accesses AudioFile base class content as fixed frames
 * of streaming linear samples.  If a codec must be assigned to perform
 * conversion to/from linear data, AudioStream will handle conversion
 * automatically.  AudioStream will also convert between mono and stereo
 * audio content.  AudioStream uses linear samples in the native
 * machine endian format and perform endian byte swapping as needed.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Audio Streaming with Linear Conversion
 */
class __EXPORT AudioStream : public AudioFile
{
protected:
    AudioCodec *codec;  // if needed
    encoded_t framebuf;
    bool streamable;
    linear_t bufferFrame;
    unsigned bufferPosition;
    unsigned bufferChannels;
    linear_t encBuffer, decBuffer;
    unsigned encSize, decSize;

    unsigned bufAudio(linear_t samples, unsigned count, unsigned size);

public:
    /**
     * Create a new audiostream object.
     */
    AudioStream();

    /**
     * Create an audio stream object and open an existing audio file.
     *
     * @param name of file to open.
     * @param mode of file access.
     * @param framing time in milliseconds.
     */
    AudioStream(const char *name, mode_t mode = modeRead, timeout_t framing = 0);

    /**
     * Create an audio stream object and a new audio file.
     *
     * @param name of file to open.
     * @param info source description for properties of new file.
     * @param exclusive access if true.
     * @param framing time in milliseconds.
     */
    AudioStream(const char *name, info_t& info, timeout_t framing = 0);

    virtual ~AudioStream();

    /**
     * Virtual for packet i/o intercept.
     *
     * @return bytes read.
     * @param data encoding buffer.
     * @param count requested.
     */
    ssize_t getBuffer(encoded_t data, size_t count);

    /**
     * Open existing audio file for streaming.
     *
     * @param name of file to open.
     * @param mode to use file.
     * @param framing timer in milliseconds.
     */
    void open(const char *name, mode_t mode = modeRead, timeout_t framing = 0);

    /**
     * Create a new audio file for streaming.
     *
     * @param name of file to create.
     * @param info source description for file properties.
     * @param exclusive true for exclusive access.
     * @param framing timing in milliseconds.
     */
    void create(const char *name, info_t& info, timeout_t framing = 0);

    /**
     * Close the currently open audio file for streaming.
     */
    void close(void);

    /**
     * flush any unsaved buffered data to disk.
     */
    void flush(void);

    /**
     * Check if the audio file may be streamed.  Files can be
     * streamed if a codec is available or if they are linear.
     *
     * @return true if streamable.
     */
    bool is_streamable(void);

    /**
     * Get the number of samples expected in a frame.
     */
    unsigned getCount(void);    // frame count

    /**
     * Stream audio data from the file and convert into an alternate
     * encoding based on the codec supplied.
     *
     * @param codec to apply before saving.
     * @param address of data to save.
     * @param frames to stream by the codec.
     * @return number of frames processed.
     */
    unsigned getEncoded(AudioCodec *codec, encoded_t address, unsigned frames = 1);

    /**
     * Stream audio data in an alternate codec into the currently
     * opened file.
     *
     * @param codec to convert incoming data from.
     * @param address of data to convert and stream.
     * @param frames of audio to stream.
     * @return number of frames processed.
     */
    unsigned putEncoded(AudioCodec *codec, encoded_t address, unsigned frames = 1);

    /**
     * Get data from the streamed file in it's native encoding.
     *
     * @param address to save encoded audio.
     * @param frames of audio to load.
     * @return number of frames read.
     */
    unsigned getEncoded(encoded_t address, unsigned frames = 1);

    /**
     * Put data encoded in the native format of the stream file.
     *
     * @param address to load encoded audio.
     * @param frames of audio to save.
     * @return number of frames written.
     */
    unsigned putEncoded(encoded_t address, unsigned frames = 1);

    /**
     * Get a packet of data from the file.  This uses the codec
     * to determine what a true packet boundry is.
     *
     * @param buffer to save encoded data.
     * @return number of bytes read as packet.
     */
    ssize_t getPacket(encoded_t data);

    /**
     * Get and automatically convert audio file data into
     * mono linear audio samples.
     *
     * @param buffer to save linear audio into.
     * @param frames of audio to read.
     * @return number of frames read from file.
     */
    unsigned getMono(linear_t buffer, unsigned frames = 1);

    /**
     * Get and automatically convert audio file data into
     * stereo (two channel) linear audio samples.
     *
     * @param buffer to save linear audio into.
     * @param frames of audio to read.
     * @return number of frames read from file.
     */
    unsigned getStereo(linear_t buffer, unsigned frames = 1);

    /**
     * Automatically convert and put mono linear audio data into
     * the audio file.  Convert to stereo as needed by file format.
     *
     * @param buffer to save linear audio from.
     * @param frames of audio to write.
     * @return number of frames written to file.
     */
    unsigned putMono(linear_t buffer, unsigned frames = 1);

    /**
     * Automatically convert and put stereo linear audio data into
     * the audio file.  Convert to mono as needed by file format.
     *
     * @param buffer to save linear audio from.
     * @param frames of audio to write.
     * @return number of frames written to file.
     */
    unsigned putStereo(linear_t buffer, unsigned frames = 1);

    /**
     * Automatically convert and put arbitrary linear mono data
     * into the audio file.  Convert to stereo and buffer incomplete
     * frames as needed by the streaming file.
     *
     * @param buffer to save linear audio from.
     * @param count of linear audio to write.
     * @return number of linear audio samples written to file.
     */
    unsigned bufMono(linear_t buffer, unsigned count);

    /**
     * Automatically convert and put arbitrary linear stereo data
     * into the audio file.  Convert to mono and buffer incomplete
     * frames as needed by the streaming file.
     *
     * @param buffer to save linear audio from.
     * @param count of linear audio to write.
     * @return number of linear audio samples written to file.
     */
    unsigned bufStereo(linear_t buffer, unsigned count);

    /**
     * Return the codec being used if there is one.
     *
     * @return codec used.
     */
    inline AudioCodec *getCodec(void)
        {return codec;};
};

/**
* Interface for managing server specific plugins.  This includes activation
* of startup and shutdown methods for modules that need to manage additional
* threads, notification of server specific events, and methods to invoke
* server actions.
* @author David Sugar <dyfet@gnutelephony.org>
*/
class __EXPORT Module : public LinkedObject, protected Env
{
protected:
    virtual void start(void);

    virtual void stop(void);

public:
    Module();

    static void startup(void);
    static void shutdown(void);
};

END_NAMESPACE

#endif

