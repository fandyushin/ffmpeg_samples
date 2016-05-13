#include <iostream>
#include <deque>
#include <algorithm>

extern "C" {
  #include <libswresample/swresample.h>
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
}

enum eNormType {
  FIRST,
  SECOND,
  THIRD
};

int main(int argc, char const *argv[]) {

  av_register_all();

  std::deque<int16_t> ampDeque;
  uint32_t dequeSize = 150;

  eNormType normType = THIRD;

  const char *sEncFileName = "./debug-audio.aac";
  AVFormatContext* formatContext = NULL;

  if (avformat_open_input(&formatContext, sEncFileName, NULL, NULL) != 0) {
    std::cout << "Error opening the file" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (avformat_find_stream_info(formatContext, NULL) < 0) {
    avformat_close_input(&formatContext);
    std::cout << "Could not read stream information" << std::endl;
    exit(EXIT_FAILURE);
	}

  av_dump_format(formatContext, 0, sEncFileName, 0);

  AVCodec *cdc = nullptr;
  int streamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &cdc, 0);
  if (streamIndex < 0) {
    avformat_close_input(&formatContext);
    std::cout << "Could not find any audio stream in the file" << std::endl;
    exit(EXIT_FAILURE);
  }

  AVStream *audioStream = formatContext->streams[streamIndex];
  AVCodecContext *codecContext = audioStream->codec;
  codecContext->codec = cdc;

  if (avcodec_open2(codecContext, codecContext->codec, NULL) != 0) {
    avformat_close_input(&formatContext);
    std::cout << "Couldn't open the context with the decoder" << std::endl;
    exit(EXIT_FAILURE);
  }

  AVPacket readingPacket;
  av_init_packet(&readingPacket);

  AVFrame *avFrame = av_frame_alloc();
  if (!avFrame) {
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    std::cout << "Error allocating the frame" << std::endl;
    exit(EXIT_FAILURE);
  }

  AVFrame* swrFrame = nullptr;

  SwrContext *swrCtx;
  swrCtx = swr_alloc_set_opts(NULL, codecContext->channel_layout,
                              AV_SAMPLE_FMT_S16, 48000,
                              codecContext->channel_layout,
                              codecContext->sample_fmt,
                              codecContext->sample_rate, 0, NULL);
  if ( !swrCtx || swr_init(swrCtx) < 0 ) {
    av_free(avFrame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    std::cout << "Error opening resample context" << std::endl;
    exit(EXIT_FAILURE);
  }

  FILE *fOut = fopen("result.pcm", "wb");
  if (!fOut) {
    av_free(avFrame);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);
    swr_free(&swrCtx);
    std::cout << "Error opening the file" << std::endl;
    exit(EXIT_FAILURE);
  }

  while (av_read_frame(formatContext, &readingPacket) == 0) {
    if (readingPacket.stream_index == audioStream->index) {
      AVPacket decodingPacket = readingPacket;
      while (decodingPacket.size > 0) {
        int gotFrame = 0;
        int result = avcodec_decode_audio4(codecContext, avFrame, &gotFrame, &decodingPacket);
        if (result >= 0 && gotFrame) {
          decodingPacket.size -= result;
          decodingPacket.data += result;

          swrFrame = av_frame_alloc();
          swrFrame->nb_samples = avFrame->nb_samples;
          swrFrame->channel_layout = codecContext->channel_layout;
          swrFrame->sample_rate = codecContext->sample_rate;
          swrFrame->format = AV_SAMPLE_FMT_S16;
          av_frame_get_buffer(swrFrame, 1);

          swr_convert_frame(swrCtx, swrFrame, avFrame);

          int dataSize = av_samples_get_buffer_size(NULL,
                                                    codecContext->channels,
                                                    avFrame->nb_samples,
                                                    AV_SAMPLE_FMT_S16,
                                                    1);
          if (dataSize < 0) {
            std::cout << "Failed to calculate data size" << std::endl;
            exit(EXIT_FAILURE);
          }
          // Normalization
          switch (normType) {
            case FIRST:
            {
              static double coef = 1.2;
              int16_t maxAmp = 0;
              int16_t *pPCM16s = (int16_t*)swrFrame->data[0];
              for (int i = 0; i < dataSize / 2; i++) {
                int16_t amp = abs(pPCM16s[i]);
                maxAmp = (amp > maxAmp) ? amp : maxAmp;
              }
              ampDeque.push_back(maxAmp);
              if (ampDeque.size() > dequeSize) {
                ampDeque.pop_front();
              }
              auto it = std::max_element(ampDeque.begin(), ampDeque.end());

              if ( (int16_t)*it * coef < 12000 ) {
                coef = (coef < 15.0) ? coef * 1.03 : coef;
              } else if ( (int16_t)*it * coef > 23000 ) {
                coef = 20000 / (int16_t)*it;
                if (coef < 1.0) coef = 1.0;
              }

              for (int i = 0; i < dataSize / 2; i++) {
                pPCM16s[i] *= coef;
              }
            }
            break;
            // Normalization 2
            case SECOND:
            {
              static double coef = 1.2;
              int16_t maxAmp = 0;
              int16_t *pPCM16s = (int16_t*)swrFrame->data[0];
              for (int i = 0; i < dataSize / 2; i++) {
                int16_t amp = abs(pPCM16s[i]);
                maxAmp = (amp > maxAmp) ? amp : maxAmp;
              }
              ampDeque.push_back(maxAmp);
              if (ampDeque.size() > dequeSize) {
                ampDeque.pop_front();
              }
              auto it = std::max_element(ampDeque.begin(), ampDeque.end());

              int16_t maxX = *it;
              if (maxX == 0) maxX = 1;
              int16_t minX = 0;
              int16_t maxY = 20000;
              int16_t minY = 0;

              for (int i = 0; i < dataSize / 2; i++) {
                pPCM16s[i] =  (maxY - minY) * ( (double)pPCM16s[i] / ( maxX -minX ) ) + minY;;
              }
            }
            break;
            case THIRD:
            {
              int16_t maxAmp = 0;
              int16_t *pPCM16s = (int16_t*)swrFrame->data[0];
              for (int i = 0; i < dataSize / 2; i++) {
                int16_t amp = abs(pPCM16s[i]);
                maxAmp = (amp > maxAmp) ? amp : maxAmp;
              }
              static double coef3 = 1.2;
              static int32_t ianorm = 300;
              double res = coef3 * (double)maxAmp;
              if( res < 10000 ) coef3 += 0.035;
      				else if( res < 20000 ) coef3 += 0.015;
      				else if( res < 28000 ) coef3 += 0.005;
      				else coef3 = 28000.0 / (double)maxAmp;

              if( coef3 >= (double)ianorm / 100.0 ) coef3 = (double)ianorm / 100.0;
              for (int i = 0; i < dataSize / 2; i++) {
                pPCM16s[i] *=  coef3;
              }
            }
            break;
          }
          fwrite(swrFrame->data[0], 1, dataSize, fOut);

          av_frame_free(&swrFrame);
          swrFrame = nullptr;
        } else {
          decodingPacket.size = 0;
          decodingPacket.data = nullptr;
        }
      }
    }
    av_free_packet(&readingPacket);
  }

  av_free(avFrame);
  avcodec_close(codecContext);
  avformat_close_input(&formatContext);
  swr_free(&swrCtx);
  fclose(fOut);

  std::cout << "Success!" << std::endl;
  return 0;
}
