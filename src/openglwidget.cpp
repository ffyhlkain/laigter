#include "openglwidget.h"
#include <QPainter>
#include <QDebug>
#include <QOpenGLVertexArrayObject>

OpenGlWidget::OpenGlWidget(QWidget *parent)
{
    m_zoom = 1.0;
    m_image = QImage(":/images/sample.png");
    normalMap = QImage(":/images/sample_n.png");
    parallaxMap = QImage(":/images/sample_p.png");
    specularMap = QImage(":/images/sample_p.png");
    laigter = QImage(":/images/laigter-texture.png");
    lightColor = QVector3D(0.0,1,0.7);
    specColor = QVector3D(0.0,1,0.7);
    ambientColor = QVector3D(1.0,1.0,1.0);
    backgroundColor = QVector3D(0.2, 0.2, 0.3);
    ambientIntensity = 0.8;
    diffIntensity = 0.6;
    specIntensity = 0.6;
    specScatter = 32;
    lightPosition = QVector3D(0.7,0.7,0.3);
    m_light = true;
    m_parallax = false;
    parallax_height = 0.03;
    tileX = false;
    tileY = false;
    m_pixelated = false;
    lightSelected = false;

    pixelSize = 3;

    QSurfaceFormat format;
    format.setSamples(16);

    setFormat(format);
}

void OpenGlWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glClearColor(backgroundColor.x(),backgroundColor.y(),backgroundColor.z(),1.0);

    m_program.create();
    m_program.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/vshader.glsl");
    m_program.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/fshader.glsl");
    m_program.link();

    lightProgram.create();
    lightProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/lvshader.glsl");
    lightProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/lfshader.glsl");
    m_program.link();

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,     0.0f, 1.0f, // bot left
        1.0f, -1.0f, 0.0f,      1.0f, 1.0f, // bot right
        1.0f,  1.0f, 0.0f,      1.0f, 0.0f,  // top right
        -1.0f,  1.0f, 0.0f,     0.0f, 0.0f // top left
    };

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).

    VAO.bind();
    VBO.create();
    VBO.bind();

    VBO.allocate(vertices,sizeof(vertices));

    int vertexLocation = m_program.attributeLocation("aPos");
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vertexLocation);

    int texCoordLocation = m_program.attributeLocation("aTexCoord");
    glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(texCoordLocation);

    m_program.bind();


    m_texture = new QOpenGLTexture(m_image);
    pixelsX = m_image.width();
    pixelsY = m_image.height();
    m_parallaxTexture = new QOpenGLTexture(parallaxMap);
    m_specularTexture = new QOpenGLTexture(specularMap);
    m_normalTexture = new QOpenGLTexture(normalMap);
    laigterTexture = new QOpenGLTexture(laigter);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_program.release();
    VAO.release();
    VBO.release();

    lightVAO.bind();
    VBO.bind();
    vertexLocation = m_program.attributeLocation("aPos");
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(vertexLocation);

    lightProgram.bind();
    texCoordLocation = m_program.attributeLocation("aTexCoord");
    glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(texCoordLocation);
    lightProgram.release();
    lightVAO.release();
    VBO.release();

    initialized();

}


void OpenGlWidget::paintGL()
{

    glClearColor(backgroundColor.x(),backgroundColor.y(),backgroundColor.z(),1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    QMatrix4x4 transform;
    float scaleX, scaleY, zoomX, zoomY;
    transform.setToIdentity();

    transform.translate(texturePosition);
    scaleX = !tileX ? sx : 1;
    scaleY = !tileY ? sy : 1;
    transform.scale(scaleX,scaleY,1);
    zoomX = !tileX ? m_zoom : 1;
    zoomY = !tileY ? m_zoom : 1;
    transform.scale(zoomX,zoomY,1);



    m_program.bind();

    VAO.bind();
    if (tileX || tileY){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }else{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }

    int i1 = m_pixelated ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
    int i2 = m_pixelated ? GL_NEAREST : GL_NEAREST;


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i2);

    glActiveTexture(GL_TEXTURE0);
    m_program.setUniformValue("light",m_light);
    m_texture->bind(0);
    m_program.setUniformValue("texture",0);
    m_program.setUniformValue("transform",transform);
    m_program.setUniformValue("pixelsX",pixelsX);
    m_program.setUniformValue("pixelsY",pixelsY);
    m_program.setUniformValue("pixelSize",pixelSize);
    m_program.setUniformValue("pixelated",m_pixelated);

    scaleX = tileX ? sx : 1;
    scaleY = tileY ? sy : 1;
    zoomX = tileX ? m_zoom : 1;
    zoomY = tileY ? m_zoom : 1;
    m_program.setUniformValue("ratio",QVector2D(1/scaleX/zoomX,1/scaleY/zoomY));

    if (tileX || tileY){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }else{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i2);
    glActiveTexture(GL_TEXTURE1);
    m_normalTexture->bind(1);
    m_program.setUniformValue("normalMap",1);

    m_parallaxTexture->bind(2);
    m_program.setUniformValue("parallaxMap",2);

    m_specularTexture->bind(3);
    m_program.setUniformValue("specularMap",3);

    //m_program.setUniformValue("viewPos",lightPosition);
    m_program.setUniformValue("viewPos",QVector3D(0,0,1));
    m_program.setUniformValue("parallax",m_parallax);
    m_program.setUniformValue("height_scale",parallax_height);

    m_program.setUniformValue("lightPos",lightPosition);
    m_program.setUniformValue("lightColor",lightColor);
    m_program.setUniformValue("specColor",specColor);
    m_program.setUniformValue("diffIntensity",diffIntensity);
    m_program.setUniformValue("specIntensity",specIntensity);
    m_program.setUniformValue("specScatter",specScatter);
    m_program.setUniformValue("ambientColor",ambientColor);
    m_program.setUniformValue("ambientIntensity",ambientIntensity);
    glDrawArrays(GL_QUADS, 0, 4);

    m_program.release();

    if (m_light){

        float x = (float)laigter.width()/width();
        float y = (float)laigter.height()/height();
        transform.setToIdentity();
        transform.translate(lightPosition);
        transform.scale(0.3*x,0.3*y,1);

        lightProgram.bind();
        lightVAO.bind();
        lightProgram.setUniformValue("transform",transform);

        laigterTexture->bind(0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glActiveTexture(GL_TEXTURE0);
        lightProgram.setUniformValue("texture",0);
        lightProgram.setUniformValue("lightColor",lightColor);
        glDrawArrays(GL_QUADS, 0, 4);

        lightProgram.release();
    }
    QImage im(m_texture->width(),m_texture->height(),QImage::Format_RGBA8888);
    im.fill(Qt::transparent);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,im.scanLine(0));
    im.save("rendered.png");
}

void OpenGlWidget::resizeGL(int w, int h)
{
    sx = (float)m_image.width()/width();
    sy = (float)m_image.height()/height();
    update();
}


void OpenGlWidget::setImage(QImage image){
    m_image = image;
    m_texture->destroy();
    m_texture->create();
    m_texture->setData(m_image);
    sx = (float)m_image.width()/width();
    sy = (float)m_image.height()/height();
    pixelsX = image.width();
    pixelsY = image.height();

}

void OpenGlWidget::setNormalMap(QImage image){
    normalMap = image;
    m_normalTexture->destroy();
    m_normalTexture->create();
    m_normalTexture->setData(normalMap);
}

void OpenGlWidget::setParallaxMap(QImage image){
    parallaxMap = image;
    m_parallaxTexture->destroy();
    m_parallaxTexture->create();
    m_parallaxTexture->setData(parallaxMap);
}

void OpenGlWidget::setSpecularMap(QImage image){
    specularMap = image;
    m_specularTexture->destroy();
    m_specularTexture->create();
    m_specularTexture->setData(specularMap);
}

void OpenGlWidget::setZoom(float zoom){
    m_zoom = zoom;
    update();
}

void OpenGlWidget::setTileX(bool x){
    tileX = x;
    texturePosition.setX(0);
    update();
}

void OpenGlWidget::setTileY(bool y){
    tileY = y;
    texturePosition.setY(0);
    update();
}

void OpenGlWidget::setParallax(bool p){
    m_parallax = p;
    update();
}

void OpenGlWidget::wheelEvent(QWheelEvent *event)
{
    QPoint degree = event->angleDelta() / 8;

    if(!degree.isNull() && degree.y()!= 0)
    {
        QPoint step = degree/qAbs(degree.y());
        setZoom(step.y() > 0 ? m_zoom * 1.1 * step.y() : - m_zoom * 0.9 * step.y());
    }
}

void OpenGlWidget::resetZoom(){
    setZoom(1.0);
    texturePosition = QVector3D(0,0,0);
}

void OpenGlWidget::fitZoom(){
    float x,y,s;
    x = (float)m_image.width()/width();
    y = (float)m_image.height()/height();
    s = qMax(x,y);
    setZoom(1/s);
    texturePosition = QVector3D(0,0,0);
}

float OpenGlWidget::getZoom(){
    return m_zoom;
}

void OpenGlWidget::mousePressEvent(QMouseEvent *event){
    if (event->buttons() & Qt::LeftButton)
    {
        float lightWidth = (float)laigter.width()/width()*0.3;//en paintgl la imagen la escalamos por 0.3
        float lightHeight = (float)laigter.height()/height()*0.3;
        float mouseX = (float)event->localPos().x()/width()*2-1;
        float mouseY = -(float)event->localPos().y()/height()*2+1;

        if (qAbs(mouseX-lightPosition.x()) < lightWidth &&
                qAbs(mouseY-lightPosition.y()) < lightHeight &&
                m_light){
            lightSelected = true;
        }else{
            textureOffset = QVector3D(mouseX,mouseY,0)- texturePosition;
        }
    }
    else if (event->buttons() & Qt::RightButton){

    }
}

void OpenGlWidget::mouseMoveEvent(QMouseEvent *event){
    if (event->buttons() & Qt::LeftButton)
    {
        float mouseX = (float)event->localPos().x()/width()*2-1;
        float mouseY = -(float)event->localPos().y()/height()*2+1;
        if (lightSelected){
            lightPosition.setX(mouseX);
            lightPosition.setY(mouseY);
        }else{
            if (!tileX)
                texturePosition.setX(mouseX-textureOffset.x());
            if (!tileY)
                texturePosition.setY(mouseY-textureOffset.y());
        }
        update();
    }
    else if (event->buttons() & Qt::RightButton){

    }
}

void OpenGlWidget::mouseReleaseEvent(QMouseEvent *event){
    lightSelected = false;
}

void OpenGlWidget::setLight(bool light){
    m_light = light;
    update();
}

void OpenGlWidget::setParallaxHeight(int height){
    parallax_height = height/1000.0;
    update();
}

void OpenGlWidget::setLightColor(QVector3D color){
    lightColor = color;
    update();
}

void OpenGlWidget::setSpecColor(QVector3D color){
    specColor = color;
    update();
}

void OpenGlWidget::setBackgroundColor(QVector3D color){
    backgroundColor = color;
    update();
}

void OpenGlWidget::setLightHeight(float height){
    lightPosition.setZ(height);
    update();
}

void OpenGlWidget::setLightIntensity(float intensity){
    diffIntensity = intensity;
    update();
}

void OpenGlWidget::setSpecIntensity(float intensity){
    specIntensity = intensity;
    update();
}

void OpenGlWidget::setSpecScatter(int scatter){
    specScatter = scatter;
    update();
}

void OpenGlWidget::setAmbientColor(QVector3D color){
    ambientColor = color;
    update();
}

void OpenGlWidget::setAmbientIntensity(float intensity){
    ambientIntensity = intensity;
    update();
}

void OpenGlWidget::setPixelated(bool pixelated){
    m_pixelated = pixelated;
    update();
}

void OpenGlWidget::setPixelSize(int size){
    pixelSize = size;
}

QImage OpenGlWidget::renderBuffer(){
    return grabFramebuffer();
}


