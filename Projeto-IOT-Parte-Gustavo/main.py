from flask import Flask, render_template, request, redirect, flash
from dotenv import load_dotenv
import psycopg2
import os

load_dotenv()

app = Flask(__name__)
app.secret_key = b'a secret key'

conn = psycopg2.connect(
    database = "projeto_b", 
    user = os.getenv("DB_LOGIN"), 
    host= os.getenv("DB_ADDRESS"),
    password = os.getenv("DB_PASSWORD"),
    port = os.getenv("DB_PORT")
)

@app.route('/')
def home():
    return render_template('./home_page.html')

@app.route('/alunos')
def alunos():
    return render_template('./alunos_page.html')

@app.route('/alunos/add', methods=['GET'])
def alunos_add_get():
    return render_template('./alunos_add.html')

@app.route('/alunos/add', methods=['POST'])
def alunos_add_post():
    cur = conn.cursor()
    matricula = request.form["matricula"]
    nome = request.form["nome"]
    data = request.form["data_nascimento"]
    genero = request.form["genero"]
    try:
        cur.execute('INSERT INTO arcondicionado_alunos (matricula, nome, data_nascimento, genero) VALUES (%s, %s, %s, %s);', (matricula, nome, data, genero))
    except:
        conn.commit()
        cur.close()
        flash("Erro ao cadastrar aluno", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Aluno cadastrado com sucesso', 'success')
    return redirect("/")

@app.route('/alunos/get')
def alunos_get():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_alunos;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./alunos_get.html', dados=data)

@app.route('/alunos/edit', methods=['GET'])
def alunos_edit_get():
    cur = conn.cursor()
    cur.execute(
        """SELECT matricula
        FROM arcondicionado_alunos;""")
    matriculas = cur.fetchall()
    conn.commit()

    cur.execute(
        """SELECT aula, turma
        FROM arcondicionado_aulas_horas;""")
    aulas = cur.fetchall()
    conn.commit()

    return render_template('./alunos_edit.html', matriculas=matriculas, aulas=aulas)

@app.route('/alunos/edit', methods=['POST'])
def alunos_edit_post():
    cur = conn.cursor()
    matricula = request.form["matricula-option"]
    aula = request.form["aula-option"]
    turma = request.form["turma-option"]
    try:
        cur.execute('INSERT INTO arcondicionado_alunos_aulas (matricula, aula, turma) VALUES (%s, %s, %s);', (matricula, aula, turma))
    except:
        conn.commit()
        cur.close()
        flash("Aluno já matriculado", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Aluno matriculado com sucesso', 'success')
    return redirect("/")

@app.route('/aulas')
def aulas():
    return render_template('./aulas_page.html')

@app.route('/aulas/add', methods=['GET'])
def aulas_add_get():
    return render_template('./aulas_add.html')

@app.route('/aulas/add', methods=['POST'])
def aulas_add_post():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    nome = request.form["nome"]
    try:
        cur.execute('INSERT INTO arcondicionado_aulas (codigo, nome) VALUES (%s, %s);', (codigo, nome))
    except:
        conn.commit()
        cur.close()
        flash("Erro ao cadastrar aula", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Aula cadastrada com sucesso', 'success')
    return redirect("/")

@app.route('/aulas/edit', methods=['GET'])
def aulas_edit_get():
    cur = conn.cursor()
    cur.execute(
        """SELECT codigo
        FROM arcondicionado_aulas;""")
    aulas = cur.fetchall()
    conn.commit()

    return render_template('./aulas_edit.html', aulas=aulas)

@app.route('/aulas/edit', methods=['POST'])
def aulas_edit_post():
    cur = conn.cursor()
    aula = request.form["aula-option"]
    turma = request.form["turma"]
    dia = request.form["dia"]
    try:
        cur.execute('INSERT INTO arcondicionado_aulas_horas (aula, turma, dia) VALUES (%s, %s, %s);', (aula, turma, dia))
    except:
        conn.commit()
        cur.close()
        flash("Turma já cadastrada", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Turma cadastrada com sucesso', 'success')
    return redirect("/")

@app.route('/aulas/get')
def aulas_get():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_aulas;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./aulas_get.html', dados=data)

@app.route('/aulas/get2')
def aulas_get2():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_aulas_horas;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./turmas_get.html', dados=data)

@app.route('/salas')
def salas():
    return render_template('./salas_page.html')

@app.route('/salas/add', methods=['GET'])
def salas_add_get():
    return render_template('./salas_add.html')

@app.route('/salas/add', methods=['POST'])
def salas_add_post():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    local = request.form["local"]
    try:
        cur.execute('INSERT INTO arcondicionado_salas (codigo, local) VALUES (%s, %s);', (codigo, local))
    except:
        conn.commit()
        cur.close()
        flash("Erro ao cadastrar sala", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Sala cadastrada com sucesso', 'success')
    return redirect("/")

@app.route('/salas/get')
def salas_get():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_salas;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./salas_get.html', dados=data)

@app.route('/aparelhos')
def aparelhos():
    return render_template('./aparelhos_page.html')

@app.route('/aparelhos/add', methods=['GET'])
def aparelhos_add_get():
    cur = conn.cursor()
    cur.execute(
        """SELECT codigo
        FROM arcondicionado_salas;""")
    salas = cur.fetchall()
    conn.commit()

    return render_template('./aparelhos_add.html', salas=salas)

@app.route('/aparelhos/add', methods=['POST'])
def aparelhos_add_post():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    sala = request.form["sala-option"]
    try:
        cur.execute('INSERT INTO arcondicionado_aparelhos (codigo, sala, qualidade) VALUES (%s, %s, %s);', (codigo, sala, "bom"))
    except:
        conn.commit()
        cur.close()
        flash("Aparelho já existe", 'error')
        return redirect("/")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Aparelho cadastrado com sucesso', 'success')
    return redirect("/")

@app.route('/aparelhos/get')
def aparelhos_get():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_aparelhos;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./aparelhos_get.html', dados=data)

if __name__ == "__main__":
    app.run(port=8000, debug=True)