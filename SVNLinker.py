import os
import unreal
import configparser
import subprocess

def main():
    # ini 파일 읽기
    config = configparser.ConfigParser(strict=False)
    ini_path = unreal.Paths.generated_config_dir() + unreal.GameplayStatics.get_platform_name() + "Editor/EditorPerProjectUserSettings.ini"
    unreal.log(f"INI 탐색 중: {ini_path}")

    # 기본값
    src_folder = unreal.Paths.project_dir() + "../SVN/"
    if os.path.exists(ini_path):
        config.read(ini_path)
        path = config.get(
            '/Script/P_RD.SVNSettings',
            'mSVNContentDir',
            fallback=None
        )
        # 값이 있을 때만 덮어쓰기
        if path: 
            src_folder = path.strip("()").split("=")[1].strip('"')
            unreal.log(f"INI 내 SVN Link 주소 발견!")

    # 심볼릭 링크 생성
    desc_folder = unreal.Paths.project_content_dir()
    link_name = "SVN"

    dest_path = os.path.join(desc_folder, link_name)

    if os.path.exists(dest_path):
        unreal.log(f"Junction 이미 존재 확인: {dest_path}")
    else:
        if not os.path.exists(src_folder):
            unreal.log_error(f'SVN 소스 폴더 없음: {src_folder}')
        else:
            try:
                cmd = f'mklink /J "{dest_path}" "{src_folder}"'
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True, encoding="cp949")

                if result.returncode == 0:
                    unreal.log(f"Junction 생성 완료: {dest_path} -> {src_folder}")
                else:
                    unreal.log_error(f"Junction 생성 실패: {result.stderr}")
                    
            except Exception as e:
                unreal.log_error(f"Junction 생성 예외: {e}")


if unreal.is_editor():
    main()
