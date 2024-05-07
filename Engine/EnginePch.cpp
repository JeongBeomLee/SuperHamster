#include "pch.h"
#include "EnginePch.h"
#include "Engine.h"

unique_ptr<Engine> gEngine = make_unique<Engine>();
unique_ptr<Vec3> cameraPos = make_unique<Vec3>();
unique_ptr<Vec3> cameraRot = make_unique<Vec3>();


wstring s2ws(const string& s)
{
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	wstring ret(buf);
	delete[] buf;
	return ret;
}

string ws2s(const wstring& s)
{
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	string r(len, '\0');
	::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

Vec3 Slerp(Vec3& start, Vec3& end, float t)
{
    // Dot product - 두 벡터 사이의 각도 계산
    float dot = start.Dot(end);

    // 역방향인 경우 처리
    if (dot < 0.0f) {
        dot = -dot;
        end = -end;
    }

    // 보간 계수 조정
    if (dot > 0.9995f) {
        return Vec3::Lerp(start, end, t);
    }

    // 각도와 sin 값 계산
    float theta = acosf(dot);
    float sinTheta = sinf(theta);

    // 보간 계수 계산
    float startWeight = sinf((1.0f - t) * theta) / sinTheta;
    float endWeight = sinf(t * theta) / sinTheta;

    // 선형 보간 결과 반환
    return start * startWeight + end * endWeight;
}
