<?php

if(isset($_POST['username']) && isset($_POST['password'])){

    $username = $_POST['username'];
    $password = $_POST['password'];

    if($username == "admin" && $password = "admin"){
        
        session_start();
        $_SESSION["username"] = $username;
        header("Location: users.php");
        die();

    }

}


?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Login Page</title>
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
  </style>
</head>
<body>
  
<div class="container">
  <div class="login-form">
    <h2 class="text-center">Login</h2>
    <form method="POST" action="">
      <div class="form-group">
        <label for="username">Username</label>
        <input type="text" class="form-control" id="username" name="username" placeholder="Enter username">
      </div>
      <div class="form-group">
        <label for="password">Password</label>
        <input type="password" class="form-control" id="password" name="password" placeholder="Password">
      </div>
      <button type="submit" class="btn btn-primary btn-block">Login</button>
    </form>
  </div>
</div>

<!-- Bootstrap JS (optional if you need JavaScript features) -->
<script src="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js"></script>
</body>
</html>
