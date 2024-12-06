[[ How to create a Estate cluster: ]]

1. Run gcloud auth login and login with stackless.dev Google Cloud Account
2. Run ./render.sh <production | test> <lowercase build_type> all
3. Run util/setup_sandbox_cluster.sh <production | test> <lowercase build_type>
4. Follow the "Next Steps" it tells you.

[[ How to release Estate: ]]

0. Increment the versions as necessary:
  nano ~/w/build/in/version.in.conf

1. Run the Test Locally
 a) Render and Build:
  cd ~/w/build && ./render.sh local release all --force-rerender && ./build.sh local release client tools
 b) In the CLion project, build all in RelWithDebInfo.
 c) Start Serenity and River interactively in CLion.
 d) Start Jayne interactively in Rider in Release mode.
 e) Run the Test:
  cd ~/w/build && ./test.sh local release

(( if the test environment doesn't exist, skip this))
2. Build, Deploy, and Test Backend
 a) Render, Build, and Deploy
  cd ~/w/build && ./render.sh test release all --force-rerender && ./build.sh test release all && ./deploy.sh test release all
 b) Test
  cd ~/w/build && ./test.sh test release

3. Build, Deploy, and Test Production Backend
 a) Render, Build, and Deploy
  cd ~/w/build && ./render.sh production release all --force-rerender && ./build.sh production release all && ./deploy.sh production release all
 b) Test
  cd ~/w/build && ./test.sh production release
 c) Publish
  cd ~/w/build && ./publish.sh production release tools client
 d) Review the package details it outputs to make sure the versions are expected and republish
  cd ~/w/build && ./publish.sh production release tools client --accept
