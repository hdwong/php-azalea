/*
 * pinyin.c
 *
 * Created by Bun Wong on 16-9-16.
 */

#include "php.h"
#include "php_azalea.h"
#include "azalea/namespace.h"
#include "azalea/azalea.h"
#include "azalea/model.h"
#include "azalea/exception.h"
#include "azalea/ext-models/pinyin.h"

#include <iconv.h>

zend_class_entry *azalea_ext_model_pinyin_ce;

/* {{{ class LocationModel methods */
static zend_function_entry azalea_ext_model_pinyin_methods[] = {
	PHP_ME(azalea_ext_model_pinyin, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PRIVATE)
	PHP_ME(azalea_ext_model_pinyin, first, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(azalea_ext_model_pinyin, token, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ AZALEA_STARTUP_FUNCTION */
AZALEA_EXT_MODEL_STARTUP_FUNCTION(pinyin)
{
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce, AZALEA_NS_NAME(PinyinModel), azalea_ext_model_pinyin_methods);
	azalea_ext_model_pinyin_ce = zend_register_internal_class_ex(&ce, azalea_model_ce);
	azalea_ext_model_pinyin_ce->ce_flags |= ZEND_ACC_FINAL;

	return SUCCESS;
}
/* }}} */

/* {{{ proto __construct */
PHP_METHOD(azalea_ext_model_pinyin, __construct) {}
/* }}} */

/* {{{ proto convertUtf8ToGbk */
static zend_string * convertUtf8ToGbk(zend_string *str)
{
	iconv_t cd;
	char *inbuf, *outbuf, *out;
	size_t inlen, outlen;
	zend_string *ret;

	cd = iconv_open("GBK", "UTF8");
	inbuf = ZSTR_VAL(str);
	inlen = ZSTR_LEN(str);
	outlen = inlen * 2;
	outbuf = (char *) ecalloc(1, outlen);
	out = outbuf;

	iconv(cd, &inbuf, &inlen, &out, &outlen);
	iconv_close(cd);
	ret = zend_string_init(outbuf, out - outbuf, 0);
	efree(outbuf);

	return ret;
}
/* }}} */

/* {{{ proto getPinyinStr */
static char * getPinyinStr(int ascii)
{
	static const int pyvalues[] = {
			-20319, -20317, -20304, -20295, -20292, -20283, -20265, -20257, -20242, -20230, -20051, -20036, -20032, -20026,
			-20002, -19990, -19986, -19982, -19976, -19805, -19784, -19775, -19774, -19763, -19756, -19751, -19746, -19741,
			-19739, -19728, -19725, -19715, -19540, -19531, -19525, -19515, -19500, -19484, -19479, -19467, -19289, -19288,
			-19281, -19275, -19270, -19263, -19261, -19249, -19243, -19242, -19238, -19235, -19227, -19224, -19218, -19212,
			-19038, -19023, -19018, -19006, -19003, -18996, -18977, -18961, -18952, -18783, -18774, -18773, -18763, -18756,
			-18741, -18735, -18731, -18722, -18710, -18697, -18696, -18526, -18518, -18501, -18490, -18478, -18463, -18448,
			-18447, -18446, -18239, -18237, -18231, -18220, -18211, -18201, -18184, -18183, -18181, -18012, -17997, -17988,
			-17970, -17964, -17961, -17950, -17947, -17931, -17928, -17922, -17759, -17752, -17733, -17730, -17721, -17703,
			-17701, -17697, -17692, -17683, -17676, -17496, -17487, -17482, -17468, -17454, -17433, -17427, -17417, -17202,
			-17185, -16983, -16970, -16942, -16915, -16733, -16708, -16706, -16689, -16664, -16657, -16647, -16474, -16470,
			-16465, -16459, -16452, -16448, -16433, -16429, -16427, -16423, -16419, -16412, -16407, -16403, -16401, -16393,
			-16220, -16216, -16212, -16205, -16202, -16187, -16180, -16171, -16169, -16158, -16155, -15959, -15958, -15944,
			-15933, -15920, -15915, -15903, -15889, -15878, -15707, -15701, -15681, -15667, -15661, -15659, -15652, -15640,
			-15631, -15625, -15454, -15448, -15436, -15435, -15419, -15416, -15408, -15394, -15385, -15377, -15375, -15369,
			-15363, -15362, -15183, -15180, -15165, -15158, -15153, -15150, -15149, -15144, -15143, -15141, -15140, -15139,
			-15128, -15121, -15119, -15117, -15110, -15109, -14941, -14937, -14933, -14930, -14929, -14928, -14926, -14922,
			-14921, -14914, -14908, -14902, -14894, -14889, -14882, -14873, -14871, -14857, -14678, -14674, -14670, -14668,
			-14663, -14654, -14645, -14630, -14594, -14429, -14407, -14399, -14384, -14379, -14368, -14355, -14353, -14345,
			-14170, -14159, -14151, -14149, -14145, -14140, -14137, -14135, -14125, -14123, -14122, -14112, -14109, -14099,
			-14097, -14094, -14092, -14090, -14087, -14083, -13917, -13914, -13910, -13907, -13906, -13905, -13896, -13894,
			-13878, -13870, -13859, -13847, -13831, -13658, -13611, -13601, -13406, -13404, -13400, -13398, -13395, -13391,
			-13387, -13383, -13367, -13359, -13356, -13343, -13340, -13329, -13326, -13318, -13147, -13138, -13120, -13107,
			-13096, -13095, -13091, -13076, -13068, -13063, -13060, -12888, -12875, -12871, -12860, -12858, -12852, -12849,
			-12838, -12831, -12829, -12812, -12802, -12607, -12597, -12594, -12585, -12556, -12359, -12346, -12320, -12300,
			-12120, -12099, -12089, -12074, -12067, -12058, -12039, -11867, -11861, -11847, -11831, -11798, -11781, -11604,
			-11589, -11536, -11358, -11340, -11339, -11324, -11303, -11097, -11077, -11067, -11055, -11052, -11045, -11041,
			-11038, -11024, -11020, -11019, -11018, -11014, -10838, -10832, -10815, -10800, -10790, -10780, -10764, -10587,
			-10544, -10533, -10519, -10331, -10329, -10328, -10322, -10315, -10309, -10307, -10296, -10281, -10274, -10270,
			-10262, -10260, -10256, -10254
	};
	static const char * pystrs[] = {
			"a", "ai", "an", "ang", "ao",
			"ba", "bai", "ban", "bang", "bao", "bei", "ben", "beng", "bi", "bian", "biao", "bie", "bin", "bing", "bo", "bu",
			"ca", "cai", "can", "cang", "cao", "ce", "ceng", "cha", "chai", "chan", "chang", "chao", "che", "chen", "cheng",
			"chi", "chong", "chou", "chu", "chuai", "chuan", "chuang", "chui", "chun", "chuo", "ci", "cong", "cou", "cu",
			"cuan", "cui", "cun", "cuo",
			"da", "dai", "dan", "dang", "dao", "de", "deng", "di", "dian", "diao", "die", "ding", "diu", "dong", "dou", "du",
			"duan", "dui", "dun", "duo",
			"e", "en", "er",
			"fa", "fan", "fang", "fei", "fen", "feng", "fo", "fou", "fu",
			"ga", "gai", "gan", "gang", "gao", "ge", "gei", "gen", "geng", "gong", "gou", "gu", "gua", "guai", "guan",
			"guang", "gui", "gun", "guo",
			"ha", "hai", "han", "hang", "hao", "he", "hei", "hen", "heng", "hong", "hou", "hu", "hua", "huai", "huan",
			"huang", "hui", "hun", "huo",
			"ji", "jia", "jian", "jiang", "jiao", "jie", "jin", "jing", "jiong", "jiu", "ju", "juan", "jue", "jun",
			"ka", "kai", "kan", "kang", "kao", "ke", "ken", "keng", "kong", "kou", "ku", "kua", "kuai", "kuan", "kuang",
			"kui", "kun", "kuo",
			"la", "lai", "lan", "lang", "lao", "le", "lei", "leng", "li", "lia", "lian", "liang", "liao", "lie", "lin",
			"ling", "liu", "long", "lou", "lu", "lv", "luan", "lue", "lun", "luo",
			"ma", "mai", "man", "mang", "mao", "me", "mei", "men", "meng", "mi", "mian", "miao", "mie", "min", "ming", "miu",
			"mo", "mou", "mu",
			"na", "nai", "nan", "nang", "nao", "ne", "nei", "nen", "neng", "ni", "nian", "niang", "niao", "nie", "nin",
			"ning", "niu", "nong", "nu", "nv", "nuan", "nue", "nuo",
			"o", "ou",
			"pa", "pai", "pan", "pang", "pao", "pei", "pen", "peng", "pi", "pian", "piao", "pie", "pin", "ping", "po", "pu",
			"qi", "qia", "qian", "qiang", "qiao", "qie", "qin", "qing", "qiong", "qiu", "qu", "quan", "que", "qun",
			"ran", "rang", "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru", "ruan", "rui", "run", "ruo",
			"sa", "sai", "san", "sang", "sao", "se", "sen", "seng", "sha", "shai", "shan", "shang", "shao", "she", "shen",
			"sheng", "shi", "shou", "shu", "shua", "shuai", "shuan", "shuang", "shui", "shun", "shuo", "si", "song", "sou",
			"su", "suan", "sui", "sun", "suo",
			"ta", "tai", "tan", "tang", "tao", "te", "teng", "ti", "tian", "tiao", "tie", "ting", "tong", "tou", "tu",
			"tuan", "tui", "tun", "tuo",
			"wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo", "wu",
			"xi", "xia", "xian", "xiang", "xiao", "xie", "xin", "xing", "xiong", "xiu", "xu", "xuan", "xue", "xun",
			"ya", "yan", "yang", "yao", "ye", "yi", "yin", "ying", "yo", "yong", "you", "yu", "yuan", "yue", "yun",
			"za", "zai", "zan", "zang", "zao", "ze", "zei", "zen", "zeng", "zha", "zhai", "zhan", "zhang", "zhao", "zhe",
			"zhen", "zheng", "zhi", "zhong", "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang", "zhui", "zhun", "zhuo", "zi",
			"zong", "zou", "zu", "zuan", "zui", "zun", "zuo"
	};
	int i;
	for (i = (sizeof(pyvalues) / sizeof(pyvalues[0]) - 1); i >= 0; i--) {
		if (pyvalues[i] <= ascii) {
			return (char *) pystrs[i];
		}
	}
	return NULL;
}
/* }}} */

/* {{{ proto getFirst */
PHP_METHOD(azalea_ext_model_pinyin, first)
{
	zend_string *str, *gbk;
	char *p, *pinyin;
	int ascii;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &str) == FAILURE) {
		return;
	}

	// utf8 只需取前面 4 字节 @see https://en.wikipedia.org/wiki/UTF-8
	str = zend_string_init(ZSTR_VAL(str), ZSTR_LEN(str) > 4 ? 4 : ZSTR_LEN(str), 0);
	gbk = convertUtf8ToGbk(str);
	p = ZSTR_VAL(gbk);
	if (*p >= 0) {
		// 非汉字处理
		zend_str_tolower(p, 1);
		if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z')) {
			RETVAL_STRINGL(p, 1);
		} else {
			RETVAL_STRINGL("#", 1);
		}
	} else {
		// 双字节处理
		ascii = *p * 256 + *(p + 1) + 256;
		if (ascii < 0 && (pinyin = getPinyinStr(ascii))) {
			RETVAL_STRING(pinyin);
		} else {
			RETVAL_STRINGL("#", 1);
		}
	}
	zend_string_release(gbk);
	zend_string_release(str);
}
/* }}} */

/* {{{ proto getToken */
PHP_METHOD(azalea_ext_model_pinyin, token)
{
	zend_string *str, *gbk;
	char *p, *end, *ret, *pinyin;
	size_t offset = 0, len;
	int ascii;
	zend_bool isLine = 1;

	if (zend_parse_parameters_throw(ZEND_NUM_ARGS(), "S", &str) == FAILURE) {
		return;
	}

	gbk = convertUtf8ToGbk(str);
	p = ZSTR_VAL(gbk);
	end = p + ZSTR_LEN(gbk);
	ret = ecalloc(1, ZSTR_LEN(str) * 6);  // 最长拼音为 6 字符
	while (p < end) {
		if (*p >= 0) {
			// 非汉字处理
			if ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
				ret[offset] = *p;
				zend_str_tolower(ret + offset, 1);
				++offset;
				isLine = 0;
			} else if (!isLine) {
				ret[offset] = '-';
				++offset;
				isLine = 1;
			}
			++p;
			continue;
		}
		// 双字节处理
		ascii = *p * 256 + *(p + 1) + 256;
		if (ascii > 0 && ascii < 160) {
			if (!isLine) {
				ret[offset]= '-';
				++offset;
				isLine = 1;
			}
			++p;
			continue;
		}
		// 汉字
		if ((pinyin = getPinyinStr(ascii))) {
			if (!isLine) {
				ret[offset] = '-';
				++offset;
			}
			strcpy(ret + offset, pinyin);
			offset += strlen(pinyin);
			ret[offset] = '-';
			++offset;
			isLine = 1;
		}
		p += 2;
	}
	zend_string_release(gbk);
	if (offset && isLine) {
		// remove last minus sign
		--offset;
	}
	RETVAL_STRINGL(ret, offset);
	efree(ret);
}
/* }}} */
