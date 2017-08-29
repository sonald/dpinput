#include "config.h"

#include <pinyin.h>

#include <fcitx/fcitx.h>
#include <fcitx-utils/utils.h>
#include <fcitx/keys.h>

#include <fcitx/ui.h>
#include <fcitx/configfile.h>
#include <fcitx/instance.h>
#include <fcitx/candidate.h>
#include <fcitx/context.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
using namespace std;

typedef struct _FcitxDPState {
    pinyin_context_t *py_ctx;
    pinyin_instance_t *py_inst;
    FcitxInstance *owner;
} FcitxDPState;

static void* DPCreate(struct _FcitxInstance* instance);
INPUT_RETURN_VALUE DoDPInput(void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE DPGetCandWords(void *arg);
INPUT_RETURN_VALUE DPGetCandWord(void *arg, FcitxCandidateWord* candWord);
char           *GetQuWei(FcitxDPState* dpstate, int iQu, int iWei);
boolean DPInit(void *arg);

FCITX_DEFINE_PLUGIN(fcitx_dpinput, ime, FcitxIMClass) = {
    DPCreate,
    NULL
};

void* DPCreate(struct _FcitxInstance* instance)
{
    FcitxDPState* dpstate = (FcitxDPState*)fcitx_utils_malloc0(sizeof(FcitxDPState));
    FcitxInstanceRegisterIM(
        instance,
        dpstate,
        "dpinput",
        ("Dpinput"),
        "dpinput",
        DPInit,
        NULL,
        DoDPInput,
        DPGetCandWords,
        NULL,
        NULL,
        NULL,
        NULL,
        100,
        "zh_CN"
    );
    dpstate->owner = instance;

    dpstate->py_ctx = pinyin_init("/usr/lib/x86_64-linux-gnu/libpinyin/data",
            "/home/sonald/.config/pinyin");

    pinyin_option_t options = PINYIN_INCOMPLETE | PINYIN_CORRECT_ALL | DYNAMIC_ADJUST;
    pinyin_set_options(dpstate->py_ctx, options);
    dpstate->py_inst = pinyin_alloc_instance(dpstate->py_ctx);

    return dpstate;
}

boolean DPInit(void *arg)
{
    FcitxDPState* dpstate = (FcitxDPState*) arg;
    FcitxInstanceSetContext(dpstate->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    return true;
}

INPUT_RETURN_VALUE DoDPInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxDPState* dpstate = (FcitxDPState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(dpstate->owner);
    char* strCodeInput = FcitxInputStateGetRawInputBuffer(input);
    INPUT_RETURN_VALUE retVal;


    retVal = IRV_TO_PROCESS;
    if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        switch (sym) {
            case FcitxKey_0:
                break;

            case FcitxKey_1: {
                int size = FcitxInputStateGetRawInputBufferSize(input);
                FcitxInputStateSetRawInputBufferSize(input, size - 1);
                strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = '\0';

                if (!FcitxInputStateGetRawInputBufferSize(input))
                    retVal = IRV_CLEAN;
                else
                    retVal = IRV_DISPLAY_CANDWORDS;
                break;
             }

            default: /* 2 - 9 */
                strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = sym;
                strCodeInput[FcitxInputStateGetRawInputBufferSize(input) + 1] = '\0';
                FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) + 1);
                /*FcitxMessages* msgs = FcitxInputStateGetPreedit(input);*/
                retVal = IRV_DISPLAY_CANDWORDS;

                break;
        }
    } else 
        retVal = IRV_TO_PROCESS;

    return retVal;
}

INPUT_RETURN_VALUE DPGetCandWord(void *arg, FcitxCandidateWord* candWord)
{
    FcitxDPState* dpstate = (FcitxDPState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(dpstate->owner);

    strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
    return IRV_COMMIT_STRING;
}

class Digits2Pinyin {
    public:
        vector<string> possiblePinyins(string digits) {
            vector<string> res;
            auto vs = letterCombinations(digits);
            for (const auto& s: vs) {
                if (isPossbilePinyin(s)) {
                    res.push_back(s);
                }
            }

            return res;
        }

    private:
        bool isPossbilePinyin(const string& s) {
            static unordered_set<string> consonants = {
                 "b", "p", "m", "f", "d", "t", "l", "n",
                 "g", "k", "h", "j", "q", "x",
                 "r", "z", "c", "s",
                 "zh", "ch", "sh"
            };

            static unordered_set<string> finals = {
                "a", "o", "e", "ai", "ei", "ao", "ou", "an", "ang", "en", "eng", "ong",
                "ua", "uo", "uai", "ui", "uan", "uang", "un", "ueng",
                "ia", "ie", "iao", "iu", "ian", "iang", "in", "ing", "iong",
                "ve", "van", "vn"

            };

            static unordered_set<string> specials = {
                "wu", "wa", "wo", "wai", "wei", "wen", "wang", "wan", "weng", 
                "yi", "ya", "ye", "yao", "yu", "yan", "yang", "yin", "ying", "yong",
                "yu", "yue", "yuan", "yun"
            };

            if (specials.find(s) != specials.end()) 
                return true;

            switch (s[0]) {
                case 'b':
                case 'p':
                case 'm':
                case 'f':
                case 'd':
                case 't':
                case 'l':
                case 'n':
                case 'g':
                case 'k':
                case 'h':
                case 'j':
                case 'q':
                case 'x':
                case 'r':
                case 'z':
                case 'c':
                case 's':
                    break;
                default:
                    return false;
            }
        
            auto i = consonants.begin();
            while (i != consonants.end()) {
                if (s.substr(0, i->size()) == *i) {
                    break;
                }
                ++i;
            }

            if (i == consonants.end()) 
                return false;

            auto final_part = s.substr(i->size());
            auto f = finals.find(final_part);
            return f != finals.end();
        }

        vector<string> letterCombinations(const string& digits) {
            static unordered_map<char, string> m {
                {'1', ""},
                    {'2', "abc"},
                    {'3', "def"},
                    {'4', "ghi"},
                    {'5', "jkl"},
                    {'6', "mno"},
                    {'7', "pqrs"},
                    {'8', "tuv"},
                    {'9', "wxyz"},
                    {'0', ""}
            };

            vector<string> vs {""};
            return iterate(vs, digits, m);
        }

        vector<string> iterate(vector<string>& vs, string next,
                unordered_map<char, string>& m) {
            if (next.size() == 0) return vs;

            char c = next[0];
            vector<string> vs2;
            for (auto& s: vs) {
                string r = m[c];
                for (int i = 0, n = r.size(); i < n; ++i)
                    vs2.push_back(s+r[i]);
            }

            return iterate(vs2, next.substr(1), m);
        }
};

static ostream& operator<<(ostream& os, const vector<string>& v)
{
    os << "[";
    for (const auto&s: v) {
        os << s << ",";
    }

    return os << "]";
}

static vector<string> DPConvertDigitsToPinyin(FcitxDPState *dpstate)
{
    FcitxInputState *input = FcitxInstanceGetInputState(dpstate->owner);
    char *raw_buf = FcitxInputStateGetRawInputBuffer(input);
    if (!raw_buf) {
        return {};
    }
    
    string s(raw_buf);
    auto res = Digits2Pinyin().possiblePinyins(s);
    cerr << res << endl;

    return res;
}

INPUT_RETURN_VALUE DPGetCandWords(void *arg)
{
    fprintf(stderr, "%s\n", __func__);
    FcitxDPState* dpstate = (FcitxDPState*)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(dpstate->owner);
    char *raw_buf = FcitxInputStateGetRawInputBuffer(input);
    if (!raw_buf) {
        return IRV_TO_PROCESS;
    }

    vector<string> cands;

    auto pys = DPConvertDigitsToPinyin(dpstate);
    for (const auto& s: pys) {
        auto len = pinyin_parse_more_full_pinyins(dpstate->py_inst, s.c_str());
        pinyin_guess_candidates(dpstate->py_inst, 0);

        guint num = 0;
        pinyin_get_n_candidate(dpstate->py_inst, &num);
        cerr << num << "cands for " << s << endl;

        for (int i = 0; i < MIN(num, 5); i++) {
            lookup_candidate_t * candidate = NULL;
            pinyin_get_candidate(dpstate->py_inst, i, &candidate);

            const char * word = NULL;
            pinyin_get_candidate_string(dpstate->py_inst, candidate, &word);

            //fprintf(stderr, "%s  ", word);
            cands.push_back(word);
        }
    }

    auto num = MIN(cands.size(), 20);
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(cand_list, num);
    FcitxCandidateWordSetChoose(cand_list, DIGIT_STR_CHOOSE);
    for (int i = 0; i < num; ++i) {
        cerr << cands[i] << " ";
        FcitxCandidateWord candWord;
        candWord.callback = DPGetCandWord;
        candWord.owner = dpstate;
        candWord.priv = NULL;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(cands[i].c_str());
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(cand_list, &candWord);
    }

    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));

    FcitxMessages *preedit = FcitxInputStateGetPreedit(input);
    FcitxMessagesSetMessageCount(preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(preedit, MSG_INPUT, raw_buf);

    return IRV_DISPLAY_CANDWORDS;
}

