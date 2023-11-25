// webpage.h
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>晾衣架 控制面板</title>
    <style>
        /* 通用样式 */
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
        }

        #video-stream {
            width: 100%; /* 默认宽度为100% */
            height: auto; /* 根据宽度调整高度 */
        }

        /* 在小屏幕上的样式 */
        @media only screen and (max-width: 600px) {
            #video-stream {
                width: 90%; /* 宽度100%适应小屏幕 */
                height: auto;
            }
        }

        /* 在中等屏幕上的样式 */
        @media only screen and (min-width: 601px) and (max-width: 1024px) {
            #video-stream {
                width: 70%; /* 宽度为屏幕宽度的70% */
                height: auto;
            }
        }

        /* 在大屏幕上的样式 */
        @media only screen and (min-width: 1025px) {
            #video-stream {
                width: 50%; /* 宽度为屏幕宽度的50% */
                height: auto;
            }
        }
    </style>
</head>
<body>
    <h1>控制面板</h1>
    <div>
        <img id="video-stream" src="" alt="摄像头未初始化">
    </div>
    <button id="control-button">改变帧大小</button>
    <button id="startMotorButton" onclick="setTrue()">启动电机</button>
    <button type="button" onclick="reverseMotor()">反转电机</button>
    <button id="stopMotorButton" onclick="setFalse()">停止电机</button>
    <button onclick="setRainSensor()">恢复自动</button>
    <button type="button" onclick="resetESP32()">重启摄像头</button>
    <p>当前温度: <span id="temperature">--</span></p>
    <p>当前湿度: <span id="humidity">--</span></p>
    <form action="/submit" method="post" onsubmit="return confirmSubmission()">
      <label for="data">输入你的通知API</label>
      <input type="text" id="data" name="data">
      <input type="submit" value="更新 API">
    </form>
    <script>
    function reverseMotor() {
    fetch('/reverseMotor')
        .then(response => response.text())
        .then(data => {
            console.log(data);
        });
    }

    function setRainSensor() {
    fetch('/setRainSensor')
        .then(response => response.text())
        .then(data => {
            console.log(data);
        });
    }

    function confirmSubmission() {
        // 获取输入框的值
        var inputData = document.getElementById('data').value;

        // 检查输入是否为空
        if (inputData.trim() === "") {
            alert("请先输入通知 API!");
            return false; // 取消提交
        }

        // 弹窗确认
        var isConfirmed = confirm("是否确认提交并更新通知API? \n频繁地提交会导致闪存寿命缩短!\nAPI: " + inputData);

        // 如果用户点击了确认按钮，则继续提交表单；否则，取消提交
        return isConfirmed;
    }

    // 获取全局变量中的视频流IP地址
    var videoStreamIp = "{globalVariable}";
    // 构建视频流的URL
    var videoStreamUrl = "http://" + videoStreamIp + ":81/stream";
    // 获取图像元素
    var videoElement = document.getElementById('video-stream');
    videoElement.src = videoStreamUrl;

    // 添加按钮点击事件
    var controlButton = document.getElementById('control-button');

    controlButton.addEventListener('click', function() {
        // 构建控制URL
        var controlUrl = "http://" + videoStreamIp + "/control?var=framesize&val=7";

        // 发送HTTP请求
        var xhr = new XMLHttpRequest();
        xhr.open('GET', controlUrl, true);
        xhr.send();

        // 在控制完成后可以执行其他操作
    });

    // 其他 JavaScript 代码
  
        // 在这里放置之前提到的 JavaScript 代码
        setInterval(function() {
            // 发起对 "/update" 路由的请求
            fetch('/update')
                .then(response => response.json())
                .then(data => {
                    // 更新网页上的温湿度显示
                    document.getElementById('temperature').innerText = data.temperature + " °C";
                    document.getElementById('humidity').innerText = data.humidity + " %";
                })
                .catch(error => {
                    console.error('Error:', error);
                });
        }, 10000);  // 每秒更新一次

        // JavaScript 代码
  function setTrue() {
    var payload = "true"; // 定义要发送的数据
    // 使用GET请求发送数据给 /setTrue
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/motor?value=" + payload, true);
    xhr.send();
  }
  // 定义一个setFalse函数
  function setFalse() {
    var payload = "false"; // 定义要发送的数据
    // 使用GET请求发送数据给 /setFalse
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/motor?value=" + payload, true);
    xhr.send();
  }

        function resetESP32() {
        // 提示用户确认
        var isConfirmed = confirm("确定要重置 ESP32-CAM 吗？");

        // 如果用户确认，发送异步请求触发重置
        if (isConfirmed) {
            var xhr = new XMLHttpRequest();
            xhr.open("POST", "/reset", true);
            xhr.send();
        }
    }
    </script>
</body>
</html>
)=====";