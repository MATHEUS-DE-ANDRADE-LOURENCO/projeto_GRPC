from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from collections.abc import Mapping as _Mapping
from typing import ClassVar as _ClassVar, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class FileChunk(_message.Message):
    __slots__ = ("content",)
    CONTENT_FIELD_NUMBER: _ClassVar[int]
    content: bytes
    def __init__(self, content: _Optional[bytes] = ...) -> None: ...

class FileRequest(_message.Message):
    __slots__ = ("file_name", "file_content", "compress_pdf_params", "convert_to_txt_params", "convert_image_format_params", "resize_image_params")
    FILE_NAME_FIELD_NUMBER: _ClassVar[int]
    FILE_CONTENT_FIELD_NUMBER: _ClassVar[int]
    COMPRESS_PDF_PARAMS_FIELD_NUMBER: _ClassVar[int]
    CONVERT_TO_TXT_PARAMS_FIELD_NUMBER: _ClassVar[int]
    CONVERT_IMAGE_FORMAT_PARAMS_FIELD_NUMBER: _ClassVar[int]
    RESIZE_IMAGE_PARAMS_FIELD_NUMBER: _ClassVar[int]
    file_name: str
    file_content: FileChunk
    compress_pdf_params: CompressPDFRequest
    convert_to_txt_params: ConvertToTXTRequest
    convert_image_format_params: ConvertImageFormatRequest
    resize_image_params: ResizeImageRequest
    def __init__(self, file_name: _Optional[str] = ..., file_content: _Optional[_Union[FileChunk, _Mapping]] = ..., compress_pdf_params: _Optional[_Union[CompressPDFRequest, _Mapping]] = ..., convert_to_txt_params: _Optional[_Union[ConvertToTXTRequest, _Mapping]] = ..., convert_image_format_params: _Optional[_Union[ConvertImageFormatRequest, _Mapping]] = ..., resize_image_params: _Optional[_Union[ResizeImageRequest, _Mapping]] = ...) -> None: ...

class CompressPDFRequest(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class ConvertToTXTRequest(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class ConvertImageFormatRequest(_message.Message):
    __slots__ = ("output_format",)
    OUTPUT_FORMAT_FIELD_NUMBER: _ClassVar[int]
    output_format: str
    def __init__(self, output_format: _Optional[str] = ...) -> None: ...

class ResizeImageRequest(_message.Message):
    __slots__ = ("width", "height")
    WIDTH_FIELD_NUMBER: _ClassVar[int]
    HEIGHT_FIELD_NUMBER: _ClassVar[int]
    width: int
    height: int
    def __init__(self, width: _Optional[int] = ..., height: _Optional[int] = ...) -> None: ...

class FileResponse(_message.Message):
    __slots__ = ("file_name", "file_content", "status_message", "success")
    FILE_NAME_FIELD_NUMBER: _ClassVar[int]
    FILE_CONTENT_FIELD_NUMBER: _ClassVar[int]
    STATUS_MESSAGE_FIELD_NUMBER: _ClassVar[int]
    SUCCESS_FIELD_NUMBER: _ClassVar[int]
    file_name: str
    file_content: FileChunk
    status_message: str
    success: bool
    def __init__(self, file_name: _Optional[str] = ..., file_content: _Optional[_Union[FileChunk, _Mapping]] = ..., status_message: _Optional[str] = ..., success: bool = ...) -> None: ...
