################################################################
######      0CPM FIRMERWARE -- CODEC SUPPORT LIBRARY      ######
################################################################


#
# Objects always required for operation as a SIP phone:
#
objs-top-codec-${CONFIG_MAINFUNCTON_SIP_PHONE} += src/codec/wrap.o

#
# Codecs for literal audio sample transfer: L8, L16
#
objs-top-codec-${CONFIG_CODEC_L8_L16} += src/codec/l8l16.o

#
# Codecs from SpanDSP: G.711, G.722, G.726
# Note: Either G.722 or G.726 implies G.711, the lowest common denominator codec
#
objs-top-codec-${CONFIG_CODEC_G711} += src/codec/spandsp/src/g711.o src/codec/spandsp/src/bitstream.o
objs-top-codec-${CONFIG_CODEC_G722} += src/codec/spandsp/src/g722.o src/codec/spandsp/src/vector_int.o
objs-top-codec-${CONFIG_CODEC_G726} += src/codec/spandsp/src/g726.o
#
# Additionally, SpanDSP supports T.38 fax
#
#TODO:T38# objs-top-$(CONFIG_CODEC_FACSIMILE) += src/codec/spandsp/src/t38_terminal.o src/codec/spandsp/src/t38_core.o src/codec/spandsp/src/t30_api.o src/codec/spandsp/src/t30.o src/codec/spandsp/src/t30_logging.o src/codec/spandsp/src/t35.o src/codec/spandsp/src/t4_rx.o src/codec/spandsp/src/t4_tx.o src/codec/spandsp/src/async.o src/codec/spandsp/src/timezone.o src/codec/spandsp/src/bit_operations.o src/codec/spandsp/src/logging.o

#
# Codec from Xiph: Speex
#
# Note: SPEEX_ULTRAWIDEBAND implies SPEEX_NARROWBAND and SPEEX_WIDEBAND
#       SPEEX_WIDEBAND      implies SPEEX_NARROWBAND
#
#TODO# objs-top-codec-${CONFIG_CODEC_SPEEX_NARROWBAND} += src/codec/speex/TODO

#
# Experimental codec by Rowetel: Codec2
#
src/codec/codec2/src/codebook.c: src/codec/codec2/src/generate_codebook.c
	make -C src/codec/codec2/src `basename $@`

src/codec/codec2/src/codebookd.c: src/codec/codec2/src/generate_codebook.c
	make -C src/codec/codec2/src `basename $@`

src/codec/codec2/src/codebookdvq.c: src/codec/codec2/src/generate_codebook.c
	make -C src/codec/codec2/src `basename $@`

objs-top-codec-${CONFIG_CODEC_CODEC2} += src/codec/codec2/src/codec2.o src/codec/codec2/src/lpc.o src/codec/codec2/src/nlp.o src/codec/codec2/src/postfilter.o src/codec/codec2/src/sine.o src/codec/codec2/src/fft.o src/codec/codec2/src/kiss_fft.o src/codec/codec2/src/interp.o src/codec/codec2/src/lsp.o src/codec/codec2/src/phase.o src/codec/codec2/src/quantise.o src/codec/codec2/src/pack.o src/codec/codec2/src/codebook.o src/codec/codec2/src/codebookd.o src/codec/codec2/src/codebookdvq.o

#
# Codec by me: RTT
#
objs-top-codec-${CONFIG_CODEC_RTT} += src/codec/rtt/recvkeys.o src/codec/rtt/sendkeys.o

#
# Codec for encryption: ZRTP
#
#TODO# objs-top-codec-${CONFIG_CODEC_ZRTP} += TODO

