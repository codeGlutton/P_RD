import os
import unreal
import configparser # 설정 파일 파싱
import subprocess # cmd 명령 내리기
import shutil # 파일 복제

def get_src_folder():
    # 기본값
    src_folder = unreal.Paths.project_dir() + "../SVN/"
    unreal.log(f"기본 SVN 소스 폴더 : {src_folder}")
    unreal.log(f"사용자 지정 SVN 소스 폴더 탐색 중...")

    # ini 파일 읽기
    config = configparser.ConfigParser(strict=False)
    ini_path = unreal.Paths.generated_config_dir() + unreal.GameplayStatics.get_platform_name() + "Editor/EditorPerProjectUserSettings.ini"
    unreal.log(f"사용자 설정 INI 탐색 : {ini_path}")

    if os.path.exists(ini_path) == True:
        config.read(ini_path)
        path = config.get(
            '/Script/P_RD.SVNSettings',
            'mSVNContentDir',
            fallback=None
        )
        # 값이 있을 때만 소스 주소 덮어쓰기
        if path: 
            src_folder = path.strip("()").split("=")[1].strip('"')
            unreal.log(f"INI 내 사용자 지정 SVN Link 주소 발견!")

    unreal.log(f"최종 SVN 소스 폴더 : {src_folder}")
    return src_folder

def get_dest_folder():
    content_folder = unreal.Paths.project_content_dir()
    link_name = "SVN"
    dest_folder = os.path.join(content_folder, link_name)

    unreal.log(f"최종 SVN 타겟 폴더 : {dest_folder}")
    return dest_folder

def get_junction_target(path):
    return os.path.realpath(path)

def make_svn_junction():
    unreal.log(f"1. SVN 에셋 폴더 파악...")

    src_folder = get_src_folder()
    dest_folder = get_dest_folder()

    # 유효 경로 체크
    if os.path.exists(src_folder) == False:
        unreal.log_error(f"SVN 소스 폴더 접근 실패: {src_folder}")
        return False

    unreal.log(f"2. Junction 검사 및 생성...")

    # 기존 Junction 체크
    if os.path.lexists(dest_folder) == True:
        post_src_norm_path = os.path.normpath(src_folder)
        pre_src_norm_path = get_junction_target(dest_folder)

        if post_src_norm_path.lower() == pre_src_norm_path.lower():
            unreal.log(f"동일 경로를 가리키는 기존의 Junction 발견: {pre_src_norm_path}")
            return True
        else:
            unreal.log(f"다른 경로를 가리키는 기존의 Junction 발견 : {pre_src_norm_path} != {post_src_norm_path}")
            try:
                cmd = f'rmdir "{dest_folder}"'
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True, encoding="cp949")

                if result.returncode == 0:
                    unreal.log(f"기존 Junction 삭제 완료")
                else:
                    unreal.log_error(f"기존 Junction 삭제 실패: {result.stderr}")
                    return False

            except Exception as e:
                unreal.log_error(f"기존 Junction 삭제 예외: {e}")
                return False
    
    # Junction 생성
    try:
        cmd = f'mklink /J "{dest_folder}" "{src_folder}"'
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, encoding="cp949")

        if result.returncode == 0:
            unreal.log(f"새로운 Junction 생성 완료: {dest_folder} -> {src_folder}")
        else:
            unreal.log_error(f"새로운 Junction 생성 실패: {result.stderr}")
            return False
            
    except Exception as e:
        unreal.log_error(f"새로운 Junction 생성 예외: {e}")
        return False

    return True

def copy_pre_commit_file():
    unreal.log(f"3. Git에 사용된 SVN 리비전 넘버 남기기 자동화...")

    src_file = unreal.Paths.project_dir() + ".githooks/pre-commit"
    dest_file = unreal.Paths.project_dir() + ".git/hooks/pre-commit"

    if os.path.exists(src_file) == True:
        unreal.log(f"Git SVN 버전 기록용 Hook 복제: {src_file} -> {dest_file}")
        shutil.copy2(src_file, dest_file)


if unreal.is_editor():
    if make_svn_junction() == True:
        copy_pre_commit_file()

