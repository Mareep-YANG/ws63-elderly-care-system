from funasr import AutoModel
from funasr.utils.postprocess_utils import rich_transcription_postprocess

# 初始化ASR模型 - 使用ModelScope自动下载，而不是本地路径
asr_model = AutoModel(
    model="paraformer-zh",  # 使用ModelScope模型名称，自动下载
    vad_model="fsmn-vad",
    vad_kwargs={"max_single_segment_time": 30000},
    device="cpu",
)


async def speak_to_text( audio_path: str ) -> str:
	"""
	将音频文件转换为文本。

	:param audio_path: 音频文件的路径
	:return: 转换后的文本
	"""
	res = asr_model.generate(input = audio_path, cache = {}, language = "zn",
	                         # "zn", "en", "yue", "ja", "ko", "nospeech"
	                         use_itn = True, batch_size_s = 60, merge_vad = True,  #
	                         merge_length_s = 15, )
	text = rich_transcription_postprocess(res[0]["text"])
	return text
