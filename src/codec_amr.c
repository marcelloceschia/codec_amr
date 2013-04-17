#include "asterisk.h"
#include "asterisk/translate.h"
#include "asterisk/module.h"
#include "asterisk/format.h"
#include "asterisk/utils.h"


enum amr_attr_keys {
	AMR_ATTR_KEY_SAMP_RATE, /*!< value is silk_attr_vals enum */
	AMR_ATTR_KEY_DTX, /*!< value is an int, 1 dtx is enabled, 0 dtx not enabled. */
	AMR_ATTR_KEY_FEC, /*!< value is an int, 1 encode with FEC, 0 do not use FEC. */
	AMR_ATTR_KEY_PACKETLOSS_PERCENTAGE, /*!< value is an int (0-100), Represents estimated packetloss in uplink direction.*/
	AMR_ATTR_KEY_MAX_BITRATE, /*!< value is an int */
	AMR_ATTR_KEY_OCTET_ALIGN,
};


#define AST_MODULE "codec_amr"

/*!
 * \brief SILK attribute structure.
 *
 * \note The only attribute that affects compatibility here is the sample rate.
 */
struct amr_attr {
	unsigned int samplerate;
	unsigned int maxbitrate;
	unsigned int dtx;
	unsigned int fec;
	unsigned int packetloss_percentage;
	unsigned int octet_align:1;
};

static enum ast_format_cmp_res amr_cmp(const struct ast_format_attr *fattr1, const struct ast_format_attr *fattr2)
{
	struct amr_attr *attr1 = (struct amr_attr *) fattr1;
	struct amr_attr *attr2 = (struct amr_attr *) fattr2;

	if (attr1->samplerate == attr2->samplerate) {
		return AST_FORMAT_CMP_EQUAL;
	}
	return AST_FORMAT_CMP_NOT_EQUAL;
}

static int amr_get_val(const struct ast_format_attr *fattr, int key, void *result)
{
	const struct amr_attr *attr = (struct amr_attr *) fattr;
	int *val = result;

	switch (key) {
	case AMR_ATTR_KEY_SAMP_RATE:
		*val = attr->samplerate;
		break;
	case AMR_ATTR_KEY_MAX_BITRATE:
		*val = attr->maxbitrate;
		break;
	case AMR_ATTR_KEY_DTX:
		*val = attr->dtx;
		break;
	case AMR_ATTR_KEY_FEC:
		*val = attr->fec;
		break;
	case AMR_ATTR_KEY_PACKETLOSS_PERCENTAGE:
		*val = attr->packetloss_percentage;
		break;
	case AMR_ATTR_KEY_OCTET_ALIGN:
		*val = attr->octet_align;
		break;
	default:
		return -1;
		ast_log(LOG_WARNING, "unknown attribute type %d\n", key);
	}
	return 0;
}

static int amr_isset(const struct ast_format_attr *fattr, va_list ap)
{
	enum silk_attr_keys key;
	const struct amr_attr *attr = (struct amr_attr *) fattr;

	for (key = va_arg(ap, int); key != AST_FORMAT_ATTR_END; key = va_arg(ap, int)) {
		switch (key) {
		case AMR_ATTR_KEY_SAMP_RATE:
			if (attr->samplerate != (va_arg(ap, int))) {
				return -1;
			}
			break;
		case AMR_ATTR_KEY_MAX_BITRATE:
			if (attr->maxbitrate != (va_arg(ap, int))) {
				return -1;
			}
			break;
		case AMR_ATTR_KEY_DTX:
			if (attr->dtx != (va_arg(ap, int))) {
				return -1;
			}
			break;
		case AMR_ATTR_KEY_FEC:
			if (attr->fec != (va_arg(ap, int))) {
				return -1;
			}
			break;
		case AMR_ATTR_KEY_PACKETLOSS_PERCENTAGE:
			if (attr->packetloss_percentage != (va_arg(ap, int))) {
				return -1;
			}
			break;
		case AMR_ATTR_KEY_OCTET_ALIGN:
			if (attr->octet_align != (va_arg(ap, int))) {
				return -1;
			}
			break;
		default:
			return -1;
			ast_log(LOG_WARNING, "unknown attribute type %d\n", key);
		}
	}
	return 0;
}
static int amr_getjoint(const struct ast_format_attr *fattr1, const struct ast_format_attr *fattr2, struct ast_format_attr *result)
{
	struct amr_attr *attr1 = (struct amr_attr *) fattr1;
	struct amr_attr *attr2 = (struct amr_attr *) fattr2;
	struct amr_attr *attr_res = (struct amr_attr *) result;
	int joint = -1;
	
	/* sample rate is the only attribute that has any bearing on if joint capabilities exist or not */
	if (attr1->samplerate == attr2->samplerate) {
		attr_res->samplerate = attr1->samplerate;
		joint = 0;
	}
	/* Take the lowest max bitrate */
	attr_res->maxbitrate = MIN(attr1->maxbitrate, attr2->maxbitrate);

	/* Only do dtx if both sides want it. DTX is a trade off between
	 * computational complexity and bandwidth. */
	attr_res->dtx = attr1->dtx && attr2->dtx ? 1 : 0;

	/* Only do FEC if both sides want it.  If a peer specifically requests not
	 * to receive with FEC, it may be a waste of bandwidth. */
	attr_res->fec = attr1->fec && attr2->fec ? 1 : 0;

	/* Use the maximum packetloss percentage between the two attributes. This affects how
	 * much redundancy is used in the FEC. */
	attr_res->packetloss_percentage = MAX(attr1->packetloss_percentage, attr2->packetloss_percentage);
	return joint;
}

static void amr_set(struct ast_format_attr *fattr, va_list ap)
{
	enum silk_attr_keys key;
	struct amr_attr *attr = (struct amr_attr *) fattr;

	for (key = va_arg(ap, int);
		key != AST_FORMAT_ATTR_END;
		key = va_arg(ap, int))
	{
		switch (key) {
		case AMR_ATTR_KEY_SAMP_RATE:
			attr->samplerate = (va_arg(ap, int));
			break;
		case AMR_ATTR_KEY_MAX_BITRATE:
			attr->maxbitrate = (va_arg(ap, int));
			break;
		case AMR_ATTR_KEY_DTX:
			attr->dtx = (va_arg(ap, int));
			break;
		case AMR_ATTR_KEY_FEC:
			attr->fec = (va_arg(ap, int));
			break;
		case AMR_ATTR_KEY_PACKETLOSS_PERCENTAGE:
			attr->packetloss_percentage = (va_arg(ap, int));
			break;
		case AMR_ATTR_KEY_OCTET_ALIGN:
			attr->octet_align = (va_arg(ap, int));
			break;
		default:
			ast_log(LOG_WARNING, "unknown attribute type %d\n", key);
		}
	}
}

int amr_format_get_samples(const struct ast_frame *f){
	int samples;
	
	
	if (!(ast_format_isset(&f->subclass.format, AMR_ATTR_KEY_SAMP_RATE, 24000, AST_FORMAT_ATTR_END))) {
		samples = f->datalen * (24000 / 4000);
	} else if (!(ast_format_isset(&f->subclass.format, AMR_ATTR_KEY_SAMP_RATE, 16000, AST_FORMAT_ATTR_END))) {
		samples = 320;
	} else if (!(ast_format_isset(&f->subclass.format, AMR_ATTR_KEY_SAMP_RATE, 12000, AST_FORMAT_ATTR_END))) {
		samples = f->datalen * (12000 / 4000);
	} else {
		samples = f->datalen * (8000 / 4000);
	}
	return samples;
}
int amr_format_get_rate(const struct ast_format *format){
	if (!(ast_format_isset(format, AMR_ATTR_KEY_SAMP_RATE, 24000, AST_FORMAT_ATTR_END))) {
		return 24000;
	} else if (!(ast_format_isset(format, AMR_ATTR_KEY_SAMP_RATE, 16000, AST_FORMAT_ATTR_END))) {
		return 16000;
	} else if (!(ast_format_isset(format, AMR_ATTR_KEY_SAMP_RATE, 12000, AST_FORMAT_ATTR_END))) {
		return 12000;
	} else {
		return 8000;
	}
}

int amr_format_parse_sdp(struct ast_format_attr *format_attr, const char *attributes){
	return 0;
}

	/*!
	 * \brief Generate SDP attribute information from an ast_format_attr structure.
	 *
	 * \note This callback should generate a full fmtp line using the provided payload number.
	 */
void amr_format_sdp_generate(const struct ast_format_attr *format_attr, unsigned int payload, struct ast_str **str){
	struct amr_attr *attr = (struct amr_attr *) format_attr;
	
	ast_str_append(str, 0, "a=fmtp:%d octet-align=%d\r\n", payload, attr->octet_align);
}

int mr_format_allowSmoother(void){
	return 0;
}

static struct ast_format_attr_interface amr_interface = {
	.id = -1,
	.format_attr_cmp 	= amr_cmp,
	.format_attr_get_joint 	= amr_getjoint,
	.format_attr_set	= amr_set,
	.format_attr_isset	= amr_isset,
	.format_attr_get_val 	= amr_get_val,
	.format_attr_sdp_parse	= NULL,
	
	.format_samples		= amr_format_get_samples,
	.format_rate		= amr_format_get_rate,
	.allowSmoother		= mr_format_allowSmoother,
	
	.format_attr_sdp_parse	= amr_format_parse_sdp,
	.format_attr_sdp_generate = amr_format_sdp_generate,
};

static int register_attributes(void)
{
	struct ast_format_list entry;
	int samplerates[] = {8000, 12000, 16000, 24000};
	int i;
	char codecName[50];
	char codecDesc[100];
	
	int fr_len = 80;
	int min_ms = 20;
	int max_ms = 20;
	int inc_ms = 20;
	int def_ms = 20;
	
	
	
	for(i=0; i < ARRAY_LEN(samplerates); i++){
		memset(&entry, 0, sizeof(entry));
		
		ast_format_set(&entry.format, amr_interface.id, 0);
		
		snprintf(codecName, sizeof(codecName), "amrwb%d", samplerates[i]/1000);
		snprintf(codecDesc, sizeof(codecDesc), "AMR-WB Custom Format %dkhz", samplerates[i]/1000);
		
		ast_copy_string(entry.name, codecName, sizeof(entry.name));
		ast_copy_string(entry.desc, codecDesc, sizeof(entry.desc));
		
		ast_format_append(&entry.format, AMR_ATTR_KEY_SAMP_RATE, samplerates[i], AST_FORMAT_ATTR_END);
		ast_add_mime_type(&entry.format, 0, "audio", "AMR-WB", samplerates[i]);
		ast_add_static_payload(-1, &entry.format, 0);
		
		entry.fr_len = 80;
		entry.min_ms = 20;
		entry.max_ms = 20;
		entry.inc_ms = 20;
		entry.def_ms = 20;
		
		ast_format_custom_add(&entry);
	}

	return 0;
}

/************ MODULE LOAD / UNLOAD **************/

static int load_module(void)
{
	if (ast_format_custom_register(&amr_interface, AST_FORMAT_TYPE_AUDIO)) {
		return AST_MODULE_LOAD_DECLINE;
	}

	ast_log(LOG_NOTICE, "format registered with id %d\n", amr_interface.id);
	register_attributes();
	
	return AST_MODULE_LOAD_SUCCESS;
}

static int unload_module(void)
{
	ast_format_attr_unreg_interface(&amr_interface);
	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER, "AMR-WB codec support",
	.load = load_module,
	.unload = unload_module,
	.load_pri = AST_MODPRI_CHANNEL_DEPEND,
);


// kate: indent-width 8; replace-tabs off; indent-mode cstyle; auto-insert-doxygen on; line-numbers on; tab-indents on; keep-extra-spaces off; auto-brackets off;