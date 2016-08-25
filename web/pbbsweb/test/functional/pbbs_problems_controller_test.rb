require 'test_helper'

class PbbsProblemsControllerTest < ActionController::TestCase
  setup do
    @pbbs_problem = pbbs_problems(:one)
  end

  test "should get index" do
    get :index
    assert_response :success
    assert_not_nil assigns(:pbbs_problems)
  end

  test "should get new" do
    get :new
    assert_response :success
  end

  test "should create pbbs_problem" do
    assert_difference('PbbsProblem.count') do
      post :create, :pbbs_problem => @pbbs_problem.attributes
    end

    assert_redirected_to pbbs_problem_path(assigns(:pbbs_problem))
  end

  test "should show pbbs_problem" do
    get :show, :id => @pbbs_problem.to_param
    assert_response :success
  end

  test "should get edit" do
    get :edit, :id => @pbbs_problem.to_param
    assert_response :success
  end

  test "should update pbbs_problem" do
    put :update, :id => @pbbs_problem.to_param, :pbbs_problem => @pbbs_problem.attributes
    assert_redirected_to pbbs_problem_path(assigns(:pbbs_problem))
  end

  test "should destroy pbbs_problem" do
    assert_difference('PbbsProblem.count', -1) do
      delete :destroy, :id => @pbbs_problem.to_param
    end

    assert_redirected_to pbbs_problems_path
  end
end
