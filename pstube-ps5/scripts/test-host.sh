#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 - <<'PYTHON'
from pathlib import Path
import os,subprocess
Path('build').mkdir(exist_ok=True)
for name in ['graphics','platform','ivf','parallel_tiler']:
    subprocess.run([os.environ.get('HOST_CXX','/usr/bin/clang++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=undefined','-Isrc',f'tests/{name}_test.cpp','-o',f'build/{name}-test'],check=True,timeout=30)
    subprocess.run([f'build/{name}-test'],check=True,timeout=20)
PYTHON

codec_host=../smarttube-ps4/build/codecs/host-install
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I"$codec_host/include" tests/software_test.cpp "$codec_host/lib/libavcodec.a" "$codec_host/lib/libswresample.a" "$codec_host/lib/libavutil.a" "$codec_host/lib/libdav1d.a" -lpthread -lm -o build/software-test
./build/software-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I"$codec_host/include" tests/playback_test.cpp "$codec_host/lib/libswscale.a" "$codec_host/lib/libavcodec.a" "$codec_host/lib/libswresample.a" "$codec_host/lib/libavutil.a" "$codec_host/lib/libdav1d.a" -lpthread -lm -o build/playback-test
./build/playback-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I"$codec_host/include" tests/av_test.cpp "$codec_host/lib/libavformat.a" "$codec_host/lib/libswscale.a" "$codec_host/lib/libavcodec.a" "$codec_host/lib/libswresample.a" "$codec_host/lib/libavutil.a" "$codec_host/lib/libdav1d.a" -lpthread -lm -o build/av-test
./build/av-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/feature_test.cpp src/shared/core.cpp src/shared/json.cpp src/shared/app_state.cpp -o build/feature-test
./build/feature-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I"$codec_host/include" tests/pcm_clock_test.cpp -lpthread -o build/pcm-clock-test
./build/pcm-clock-test
for name in oauth tv_oauth live_hls; do
    /usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc/shared "tests/${name}_test.cpp" src/shared/json.cpp src/shared/oauth.cpp src/shared/tv_oauth.cpp src/shared/live_hls.cpp -o "build/${name}-test"
    "./build/${name}-test"
done
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -Ithird_party tests/account_beta_test.cpp src/shared/oauth.cpp src/shared/tv_oauth.cpp src/shared/json.cpp src/shared/core.cpp src/shared/app_ui.cpp src/shared/app_state.cpp -o build/account-beta-test
./build/account-beta-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/features_v013_test.cpp src/shared/core.cpp src/shared/json.cpp -o build/features-v013-test
./build/features-v013-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/thumbnail_async_test.cpp src/shared/json.cpp -lpthread -o build/thumbnail-async-test
./build/thumbnail-async-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I../vendor/pacbrew-v0.40.2/user/homebrew/include tests/network_preferences_test.cpp src/shared/json.cpp -o build/network-preferences-test
./build/network-preferences-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc tests/content_services_test.cpp src/shared/json.cpp -o build/content-services-test
./build/content-services-test

/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -I../vendor/pacbrew-v0.40.2/user/homebrew/include -I"$codec_host/include" tests/stream_reconnect_test.cpp src/shared/core.cpp src/shared/json.cpp src/shared/live_hls.cpp -lpthread -lcurl -o build/stream-reconnect-test
./build/stream-reconnect-test
/usr/bin/clang++ -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -Isrc -Ithird_party tests/ui_preview.cpp src/shared/json.cpp src/shared/core.cpp src/shared/app_state.cpp src/shared/app_ui.cpp -o build/ui-preview
./build/ui-preview
