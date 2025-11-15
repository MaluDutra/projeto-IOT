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

@app.route('/alunos/delete', methods=['POST'])
def alunos_delete_post():
    cur = conn.cursor()
    matricula = request.form["matricula"]
    try:
        cur.execute('DELETE FROM arcondicionado_alunos WHERE matricula = %s;', (matricula,))
    except Exception as e:
        print(str(e))
        conn.commit()
        cur.close()
        flash("Erro ao remover aluno", 'error')
        return redirect("/alunos/get")
    # Make the changes to the database persistent
    conn.commit()
    # Close cursor and communication with the database
    cur.close()
    flash(f'Aluno removido com sucesso', 'success')
    return redirect("/alunos/get")

@app.route('/alunos/edit2', methods=['GET', 'POST'])
def alunos_edit2():
    cur = conn.cursor()
    if request.method == 'POST':
        # Check if it's the form to get the edit page or to update the student
        print(request.form)
        matricula = request.form["matricula"]
        nome = request.form["nome"]
        data = request.form["data_nascimento"]
        genero = request.form["genero"]
        id_chat = request.form["id_chat"]
        try:
            cur.execute('UPDATE arcondicionado_alunos SET nome = %s, data_nascimento = %s, genero = %s, id_chat = %s WHERE matricula = %s;', (nome, data, genero, id_chat, matricula))
        except Exception as e:
            print(str(e))
            conn.commit()
            cur.close()
            flash("Erro ao editar aluno", 'error')
            return redirect("/alunos/get")
        conn.commit()
        cur.close()
        flash(f'Aluno editado com sucesso', 'success')
        return redirect("/alunos/get")
    else:
        matricula = request.args.get("matricula")
    
    cur.execute('SELECT * FROM arcondicionado_alunos WHERE matricula = %s;', (matricula,))
    aluno = cur.fetchone()
    conn.commit()
    cur.close()
    if aluno:
        return render_template('./alunos_edit2.html', aluno=aluno)
    else:
        flash("Aluno não encontrado", 'error')
        return redirect("/alunos/get")
    
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

@app.route('/aulas/edit2', methods=['GET', 'POST'])
def aulas_edit2():
    cur = conn.cursor()
    # Check if it's the form to get the edit page or to update the aula
    if 'nome' in request.form:  # This is the update form
        codigo = request.form["codigo"]
        nome = request.form["nome"]
        try:
            cur.execute('UPDATE arcondicionado_aulas SET nome = %s WHERE codigo = %s;', (nome, codigo))
        except Exception as e:
            print(str(e))
            conn.commit()
            cur.close()
            flash("Erro ao editar aula", 'error')
            return redirect("/aulas/get")
        conn.commit()
        cur.close()
        flash(f'Aula editada com sucesso', 'success')
        return redirect("/aulas/get")
    else:
        codigo = request.args.get("codigo")
    
    cur.execute('SELECT * FROM arcondicionado_aulas WHERE codigo = %s;', (codigo,))
    aula = cur.fetchone()
    conn.commit()
    cur.close()
    if aula:
        return render_template('./aulas_edit2.html', aula=aula)
    else:
        flash("Aula não encontrada", 'error')
        return redirect("/aulas/get")

@app.route('/aulas/delete', methods=['POST'])
def aulas_delete():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    try:
        cur.execute('DELETE FROM arcondicionado_aulas WHERE codigo = %s;', (codigo,))
    except Exception as e:
        print(str(e))
        conn.commit()
        cur.close()
        flash("Erro ao remover aula", 'error')
        return redirect("/aulas/get")
    conn.commit()
    cur.close()
    flash(f'Aula removida com sucesso', 'success')
    return redirect("/aulas/get")

@app.route('/aulas/get2')
def aulas_get2():
    cur = conn.cursor()
    cur.execute('SELECT * FROM arcondicionado_aulas_horas;')
    data = cur.fetchall()
    conn.commit()

    return render_template('./turmas_get.html', dados=data)

@app.route('/turmas/edit2', methods=['GET', 'POST'])
def turmas_edit2():
    cur = conn.cursor()
    # Check if it's the form to get the edit page or to update the turma
    if 'dia' in request.form:  # This is the update form
        aula = request.form["aula"]
        turma = request.form["turma"]
        dia = request.form["dia"]
        try:
            cur.execute('UPDATE arcondicionado_aulas_horas SET dia = %s WHERE aula = %s AND turma = %s;', (dia, aula, turma))
        except Exception as e:
            print(str(e))
            conn.commit()
            cur.close()
            flash("Erro ao editar turma", 'error')
            return redirect("/aulas/get2")
        conn.commit()
        cur.close()
        flash(f'Turma editada com sucesso', 'success')
        return redirect("/aulas/get2")
    else:
        aula = request.args.get("aula")
        turma = request.args.get("turma")
        dia = request.args.get("dia")
    
    cur.execute('SELECT * FROM arcondicionado_aulas_horas WHERE aula = %s AND turma = %s AND dia = %s;', (aula, turma, dia))
    turma_data = cur.fetchone()
    
    conn.commit()
    cur.close()
    if turma_data:
        return render_template('./turmas_edit2.html', turma=turma_data)
    else:
        flash("Turma não encontrada", 'error')
        return redirect("/aulas/get2")

@app.route('/turmas/delete', methods=['POST'])
def turmas_delete():
    cur = conn.cursor()
    aula = request.form["aula"]
    turma = request.form["turma"]
    dia = request.form["dia"]
    try:
        cur.execute('DELETE FROM arcondicionado_aulas_horas WHERE aula = %s AND turma = %s AND dia = %s;', (aula, turma, dia))
    except Exception as e:
        print(str(e))
        conn.commit()
        cur.close()
        flash("Erro ao remover turma", 'error')
        return redirect("/aulas/get2")
    conn.commit()
    cur.close()
    flash(f'Turma removida com sucesso', 'success')
    return redirect("/aulas/get2")

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

@app.route('/salas/edit2', methods=['GET', 'POST'])
def salas_edit2():
    cur = conn.cursor()
    # Check if it's the form to get the edit page or to update the sala
    if 'local' in request.form:  # This is the update form
        codigo = request.form["codigo"]
        local = request.form["local"]
        try:
            cur.execute('UPDATE arcondicionado_salas SET local = %s WHERE codigo = %s;', (local, codigo))
        except Exception as e:
            print(str(e))
            conn.commit()
            cur.close()
            flash("Erro ao editar sala", 'error')
            return redirect("/salas/get")
        conn.commit()
        cur.close()
        flash(f'Sala editada com sucesso', 'success')
        return redirect("/salas/get")
    else:
        codigo = request.args.get("codigo")
    
    cur.execute('SELECT * FROM arcondicionado_salas WHERE codigo = %s;', (codigo,))
    sala = cur.fetchone()
    conn.commit()
    cur.close()
    if sala:
        return render_template('./salas_edit2.html', sala=sala)
    else:
        flash("Sala não encontrada", 'error')
        return redirect("/salas/get")

@app.route('/salas/delete', methods=['POST'])
def salas_delete():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    try:
        cur.execute('DELETE FROM arcondicionado_salas WHERE codigo = %s;', (codigo,))
    except Exception as e:
        print(str(e))
        conn.commit()
        cur.close()
        flash("Erro ao remover sala", 'error')
        return redirect("/salas/get")
    conn.commit()
    cur.close()
    flash(f'Sala removida com sucesso', 'success')
    return redirect("/salas/get")

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
        cur.execute('INSERT INTO arcondicionado_aparelhos (codigo, sala, qualidade) VALUES (%s, %s, %s);', (codigo, sala, "Bom"))
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

@app.route('/aparelhos/edit2', methods=['GET', 'POST'])
def aparelhos_edit2():
    cur = conn.cursor()
    # Check if it's the form to get the edit page or to update the device
    if 'sala' in request.form and 'qualidade' in request.form:  # This is the update form
        codigo = request.form["codigo"]
        sala = request.form["sala"]
        qualidade = request.form["qualidade"]
        try:
            cur.execute('UPDATE arcondicionado_aparelhos SET sala = %s, qualidade = %s WHERE codigo = %s;', (sala, qualidade, codigo))
        except Exception as e:
            print(str(e))
            conn.commit()
            cur.close()
            flash("Erro ao editar aparelho", 'error')
            return redirect("/aparelhos/get")
        conn.commit()
        cur.close()
        flash(f'Aparelho editado com sucesso', 'success')
        return redirect("/aparelhos/get")
    else:
        codigo = request.args.get("codigo")
    
    cur.execute('SELECT * FROM arcondicionado_aparelhos WHERE codigo = %s;', (codigo,))
    aparelho = cur.fetchone()
    
    conn.commit()
    cur.close()
    if aparelho:
        return render_template('./aparelhos_edit2.html', aparelho=aparelho)
    else:
        flash("Aparelho não encontrado", 'error')
        return redirect("/aparelhos/get")

@app.route('/aparelhos/delete', methods=['POST'])
def aparelhos_delete():
    cur = conn.cursor()
    codigo = request.form["codigo"]
    try:
        cur.execute('DELETE FROM arcondicionado_aparelhos WHERE codigo = %s;', (codigo,))
    except Exception as e:
        print(str(e))
        conn.commit()
        cur.close()
        flash("Erro ao remover aparelho", 'error')
        return redirect("/aparelhos/get")
    conn.commit()
    cur.close()
    flash(f'Aparelho removido com sucesso', 'success')
    return redirect("/aparelhos/get")

if __name__ == "__main__":
    app.run(port=8000, debug=True)