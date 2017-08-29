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
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <regex>
using namespace std;

struct PinyinBase {
    unordered_set<string> all;
};

typedef struct _FcitxDPState {
    pinyin_context_t *py_ctx;
    pinyin_instance_t *py_inst;
    FcitxInstance *owner;
    PinyinBase *py;
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

    pinyin_option_t options = PINYIN_INCOMPLETE | PINYIN_CORRECT_ALL | 
        USE_DIVIDED_TABLE | USE_RESPLIT_TABLE |
        DYNAMIC_ADJUST;
    pinyin_set_options(dpstate->py_ctx, options);
    dpstate->py_inst = pinyin_alloc_instance(dpstate->py_ctx);

    // load pinyin table
    dpstate->py = new PinyinBase;

    char *pkgdatadir = fcitx_utils_get_fcitx_path("pkgdatadir");
    string file(pkgdatadir);
    file += "/dpinput/pinyin.txt";

    cerr << file << endl;
    if (ifstream py_fs{file, std::ios::in}) {
        for (std::array<char, 10> a; py_fs.getline(&a[0], 10);) {
            dpstate->py->all.emplace(a.data());
        }
    }

    free(pkgdatadir);

    return dpstate;
}

boolean DPInit(void *arg)
{
    FcitxDPState* dpstate = (FcitxDPState*) arg;
    FcitxInstanceSetContext(dpstate->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");

    return true;
}

static bool dp_choose = false;
INPUT_RETURN_VALUE DoDPInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxDPState* dpstate = (FcitxDPState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(dpstate->owner);
    char* strCodeInput = FcitxInputStateGetRawInputBuffer(input);
    INPUT_RETURN_VALUE retVal;

    retVal = IRV_TO_PROCESS;
    if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        if (dp_choose) {
            switch (sym) {
                case FcitxKey_0:
                    dp_choose = !dp_choose;
                    break;
                case FcitxKey_8: /* turn up page */
                {
                    cerr << "turn up page" << endl;
                    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
                    if (FcitxCandidateWordHasPrev(cand_list)) {
                        FcitxCandidateWordGoPrevPage(cand_list);
                    }
                    retVal = IRV_FLAG_UPDATE_INPUT_WINDOW;
                    break;
                }
                case FcitxKey_9: /* turn down page */
                {
                    cerr << "turn down page" << endl;
                    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
                    if (FcitxCandidateWordHasNext(cand_list)) {
                        FcitxCandidateWordGoNextPage(cand_list);
                    }
                    retVal = IRV_FLAG_UPDATE_INPUT_WINDOW;
                    break;
                }

                default: /* 1-7 */
                {
                    cerr << "choose by index: " << sym - 0x30 << endl;
                    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
                    FcitxCandidateWordChooseByIndex(cand_list, sym - 0x30 - 1);
                    retVal = IRV_COMMIT_STRING;
                    break;
                }
            }

        } else {
            switch (sym) {
                case FcitxKey_0:
                    dp_choose = !dp_choose;
                    break;

                case FcitxKey_1: {
                    int size = FcitxInputStateGetRawInputBufferSize(input);
                    if (size) {
                        FcitxInputStateSetRawInputBufferSize(input, size - 1);
                        strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = '\0';
                    }

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

enum PinyinResultType {
    JianPin,
    Single, // single hanzi
    Multi, // maybe many hanzi
};

struct PinyinResult {
    string py;
    PinyinResultType type;

    PinyinResult(string s, PinyinResultType ty): py{s}, type{ty} {}
};

class Digits2Pinyin {
    const unordered_set<string> consonants {
        "b", "p", "m", "f", "d", "t", "l", "n",
        "g", "k", "h", "j", "q", "x",
        "r", "z", "c", "s",
        "zh", "ch", "sh"
    };

    const unordered_set<string> finals {
        "a", "o", "e", "ai", "ei", "ao", "ou", "an", "ang", "en", "eng", "ong",
            "ua", "uo", "uai", "ui", "uan", "uang", "un", "ueng",
            "i", "ia", "ie", "iao", "iu", "ian", "iang", "in", "ing", "iong",
            "ve", "van", "vn"

    };

    const unordered_set<string> specials {
        "a", "o", "e", "ai", "ei", "ao", "ou", "an", "ang", "en", "eng", "ong",
        "wu", "wa", "wo", "wai", "wei", "wen", "wang", "wan", "weng", 
            "yi", "ya", "ye", "yao", "yu", "yan", "yang", "yin", "ying", "yong",
            "yu", "yue", "yuan", "yun"
    };

    public:
        vector<PinyinResult> possiblePinyins(PinyinBase* db, string digits) {
            vector<PinyinResult> res;
            auto vs = letterCombinations(digits);
            for (const auto& s: vs) {
                if (isJianpin(s)) {
                    res.emplace_back(s, PinyinResultType::JianPin);
                } else if (db->all.find(s) != db->all.end()) {
                    res.emplace_back(s, PinyinResultType::Single);
                } else if (isPinyinSequence(s)) {
                    res.emplace_back(s, PinyinResultType::Multi);
                } 
            }

            return res;
        }

    private:
        //可能是简拼
        bool isJianpin(const string& s) {
            return false; // disabled

            string vowels = "aeiou"; 
            for (auto c: s) {
                if (vowels.find(c) != string::npos)
                    return false;
            }

            cerr << s << " may be jianpin" << endl;
            return true;
        }

        bool isPinyinSequence(const string& s) {
            regex r("[^aoeiuv]?h?[iuv]?(ai|ei|ao|ou|er|ang?|eng?|ong|a|o|e|i|u|ng|n)?");
            std::smatch m;

            string ss(s);
            while (ss.size() && regex_search(ss, m, r)) {
                if (m.str().size() <= 1) 
                    return false;
                //cerr << "matched: " << m.str() << endl;
                ss = m.suffix();
            }
            return ss.size() == 0;
        }

        //useless
        bool isPossbilePinyin(const string& s) {
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

INPUT_RETURN_VALUE DPGetCandWords(void *arg)
{
    fprintf(stderr, "%s\n", __func__);
    FcitxDPState* dpstate = (FcitxDPState*)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(dpstate->owner);
    char *raw_buf = FcitxInputStateGetRawInputBuffer(input);
    if (!raw_buf) {
        return IRV_TO_PROCESS;
    }

    vector<string> cands[3];

    auto pys = Digits2Pinyin().possiblePinyins(dpstate->py, raw_buf);

    for (const auto& s: pys) {
        auto len = pinyin_parse_more_full_pinyins(dpstate->py_inst, s.py.c_str());
        if (!len) continue;

        guint take = 0;
        int idx = 0;
        switch (s.type) {
            case PinyinResultType::JianPin:
                if (!pinyin_guess_sentence_with_prefix(dpstate->py_inst, "")) 
                    continue;
                take = 2;
                idx = 1;
                break;

            case PinyinResultType::Single:
                if (!pinyin_guess_full_pinyin_candidates(dpstate->py_inst, 0))
                    continue;
                take = 9;
                idx = 0;
                break;

            case PinyinResultType::Multi:
                if (!pinyin_guess_sentence_with_prefix(dpstate->py_inst, "")) 
                    continue;
                if (!pinyin_guess_full_pinyin_candidates(dpstate->py_inst, 0))
                    continue;
                take = 15;
                idx = 2;
                break;
        }


        guint num = 0;
        pinyin_get_n_candidate(dpstate->py_inst, &num);
        cerr << num << " cands for " << s.py << ", type " << s.type << ", take " << take << endl;

        //TODO: sort candidates?
        for (int i = 0; i < MIN(num, take); i++) {
            lookup_candidate_t * candidate = NULL;
            pinyin_get_candidate(dpstate->py_inst, i, &candidate);

            const char * word = NULL;
            pinyin_get_candidate_string(dpstate->py_inst, candidate, &word);

            //fprintf(stderr, "%s  ", word);
            cands[idx].push_back(word);
        }
    }

    auto num = 100;
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetPageSize(cand_list, 7);
    FcitxCandidateWordSetChoose(cand_list, DIGIT_STR_CHOOSE);
    for (int idx = 0; num && idx < 3; idx++) {
        for (int i = 0; num && i < cands[idx].size(); ++i) {
            num--;
            cerr << cands[idx][i] << " ";
            FcitxCandidateWord candWord;
            candWord.callback = DPGetCandWord;
            candWord.owner = dpstate;
            candWord.priv = NULL;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(cands[idx][i].c_str());
            candWord.wordType = MSG_OTHER;
            FcitxCandidateWordAppend(cand_list, &candWord);
        }
    }

    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));

    FcitxMessages *preedit = FcitxInputStateGetPreedit(input);
    FcitxMessagesSetMessageCount(preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(preedit, MSG_INPUT, raw_buf);

    return IRV_DISPLAY_CANDWORDS;
}

