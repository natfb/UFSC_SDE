async function envia() {
  let senha = document.getElementById("senha").value;
  let id = document.getElementById("ID").value;

  let tmp = await fetch("/senha", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      ID: id,
      senha: senha,
    }),
  });

  let resposta = await tmp.json();
  console.log(resposta);
  if (resposta.Status == "Falha") {
    document.getElementById("msg").innerHTML = "Senha inválida";
  } else document.location.href = "sucesso.html";
}
