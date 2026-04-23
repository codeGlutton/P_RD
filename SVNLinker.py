import os
import unreal
import configparser
import subprocess

def main():
    # ini 파일 읽기
    config = configparser.ConfigParser()
    config.read(unreal.Paths.project_config_dir() + "DefualtLink.ini")
    src_folder = unreal.Paths.project_dir() + config.get('SharedSVNContent.SharedSVNContent', 'Path')

    if not src_folder:
        unreal.log_error("SharedSVNContent 값이 ini에 없음")
        raise Exception("Config Missing")

    # 심볼릭 링크 생성
    desc_folder = unreal.Paths.project_content_dir()
    link_name = "SVN"

    dest_path = os.path.join(desc_folder, link_name)

    if os.path.exists(dest_path):
        unreal.log(f"이미 존재 확인: {dest_path}")
    else:
        if not os.path.exists(src_folder):
            unreal.log_error(f'소스 폴더 없음: {src_folder}')
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
